/*------------------------------------------------------------------------------------------------------------------
	--	SOURCE FILE : Command.cpp - An application that will receive data, and prints out the statistics for the transaction: 
	--					(1) The total bytes received from the socket
	--					(2) The total time it took to complete the entire transaction
	--					(3) Packets received/Packets expected if on UDP
	--					(4) Save the incoming data if the user chooses to do so into a .txt file
	--	
	--	PROGRAM :	4985_Assn2GUISrv.exe
	--	
	--	FUNCTIONS :
	--	
	--		int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)
	--	
	--		LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	--	
	--		void printScreen(HWND hwnd, char *buffer)
	--	
	--		void wipeScreen(HWND hwnd);
	--		
	--		void runTcpLoop(SOCKET s, bool upload);
	--		
	--		void runUdpLoop(SOCKET s, bool upload);
	--		
	--		int countActualBytes(char * buf, int len);
	--	
	--		DWORD WINAPI runTCPthread(LPVOID upload);
	--		
	--		DWORD WINAPI printTCPthread(LPVOID hwnd);
	--		
	--		DWORD WINAPI runUDPthread(LPVOID upload);
	--		
	--		DWORD WINAPI printUDPthread(LPVOID hwnd);
	--	
	--		INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	--	
	--	DATE: FEB 12, 2019
	--	
	--	REVISIONS : 
	--			FEB 12, 2019: Comments
	--			FEB 11, 2018: Thread function for print
	--			FEB 10, 2019: UDP implemented - packets
	--			FEB 09, 2019: UDP implemented - file
	--			FEB 08, 2019: TCP implemented - packets, tested
	--			FEB 06, 2019: Interactions with menu items, TCP implemented - file, tested
	--			FEB 04, 2019: Created Menu items
	--			FEB 02, 2019: Created
	--	
	--	DESIGNER : Jacky LI
	--	
	--	PROGRAMMER : Jacky Li
	--	
	--	NOTES :
	--		The program will listen on sockets using TCP/UDP protocol, and collect statistics on the transaction
	--		The program will continue to listen on the specified ports, until quit, or a new one has been chosen
-------------------------------------------------------------------------------------------------------------------*/

#define STRICT
#include "command.h"
#include "Client.h"

#define TOTAL_TIMEOUT 5000
#define MAX_ACCEPT_CLIENTS 12

static TCHAR CmdModName[] = TEXT("TCP/UDP Receiver");
//HWND cmdhwnd; // Window handler for main window
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void printScreen(HWND hwnd, char *buffer);

// Query params, along with file
char queryPortStr[INPUT_MAX_CHAR]{ '\0' };
char queryPacketSizeStr[INPUT_MAX_CHAR]{ '\0' };
QueryParams serverTCPParams;
QueryParams serverUDPParams;
QueryParams clientUDPParams;
QueryParams clientTgtParams;

// Packet size
char * sbuf;
int buflen;

BOOL CreateSocketInformation(SOCKET s);
void FreeSocketInformation(DWORD Event);

// Custom functions
void runTcpLoop(SOCKET s, bool upload);
int runUdpLoop(SOCKET s, bool upload);

// Thread functions
DWORD WINAPI runTCPthread(LPVOID upload);
DWORD WINAPI printTCPthread(LPVOID hwnd);
DWORD WINAPI runUDPthread(LPVOID upload);
DWORD WINAPI printUDPthread(LPVOID hwnd);
DWORD WINAPI runAcceptThread(LPVOID acceptSocket);

DWORD EventTotal = 0;
WSAEVENT				EventArray[WSA_MAXIMUM_WAIT_EVENTS];
LPSOCKET_INFORMATION	SocketArray[WSA_MAXIMUM_WAIT_EVENTS];

// WSA Events, only need write_event for now
WSANETWORKEVENTS	NetworkEvents;
DWORD				Event;
SOCKET				ListenSocket;
SOCKET				AcceptSocket;
SOCKET				ClientSocket;
WSADATA				WSAData;
// Resuable by both client and server
SOCKADDR_IN			serverTCP;
SOCKADDR_IN			serverUDP;
SOCKADDR_IN			clientUDP;

// Print event
HANDLE	print_evt;
DWORD	thread_srv_id;
DWORD	thread_print_id;

DWORD	acceptedClients[MAX_ACCEPT_CLIENTS];
DWORD	thread_accept_id; //Throw this into array for multiple clients

HANDLE	h_thread_srv;
HANDLE	h_thread_print;

HANDLE	acceptedThreadHandles[MAX_ACCEPT_CLIENTS];
HANDLE  h_thread_accept; //Throw this into array for multiple clients

DWORD Flags;
DWORD RecvBytes;

// Transfer statistics
unsigned long totalBytes = 0;
unsigned int totalTime = 0;
int expected_packets = 0;
int recv_packets = 0;
std::chrono::time_point<std::chrono::high_resolution_clock> start;
std::chrono::time_point<std::chrono::high_resolution_clock> end;

//unused right now
char queryResult[MAXGETHOSTSTRUCT];
char inputTextBuffer[INPUT_MAX_CHAR];

// Text metrics for printing to screen
TEXTMETRIC tm;
int xPosition;
int yPosition;

u_long lTTL;


/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: setupTCPSrv
--
--    DATE : FEB 08, 2019
--
--    REVISIONS :
--    		(FEB 08, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : int setupTCPSrv()
--
--    RETURNS : int
--				0 if everything is successful
--
--    NOTES :
--			Sets up all the global variables this program needs to make it ready to accept connections from a socket,
--			using the TCP protocol
----------------------------------------------------------------------------------------------------------------------*/
int setupTCPSrv() {
	int err;
	if (serverTCPParams.portStr[0] == '\0') {
		return 2;
	}
	if (serverTCPParams.packetSizeStr[0] == '\0') {
		return 3;
	}
	// Startup
	if ((err = WSAStartup(0x0202, &WSAData)) != 0) //No usable DLL
	{
		exit(err);
	}
	// Create Socket
	if ((ListenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		exit(600);
	}

	//CreateSocketInformation(ListenSocket);

	// Bind socket
	serverTCP.sin_family = AF_INET;
	serverTCP.sin_addr.s_addr = htonl(INADDR_ANY);
	serverTCP.sin_port = htons(atoi(serverTCPParams.portStr));
	if (bind(ListenSocket, (PSOCKADDR)&serverTCP, sizeof(serverTCP)) == SOCKET_ERROR)
	{
		printf("bind() failed with error %d\n", WSAGetLastError());
		return (601);
	}


	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: runTcpLoop
--
--    DATE : FEB 08, 2019
--
--    REVISIONS :
--    		(FEB 08, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : void runTcpLoop(SOCKET Listen, bool upload)
--			SOCKET Listen:		The socket which this function will setup and listen on
--			bool upload:		Whether or not to have the listener server save the file
--
--    RETURNS : void
--
--    NOTES :
--			Runs the loop to listen onto the TCP port specified by the windows GUI, continues to run until program
--			ends, only 1 client allowed at any given moment.
--			Exits the entire program if any of the process in setting up is unsuccessful
----------------------------------------------------------------------------------------------------------------------*/
void runTcpLoopOld(SOCKET Listen, bool upload) {
	if (WSAEventSelect(Listen, EventArray[EventTotal - 1], FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		OutputDebugString(convertErrString("WSAEventSelect() failed with error", WSAGetLastError()));
		return;
	}
	// Variables
	std::ofstream out_file;

	// listen to the port, error check
	if (listen(Listen, SOMAXCONN) == SOCKET_ERROR)
	{
		OutputDebugString(convertErrString("listen() failed with error", WSAGetLastError()));
		return;
	}

	// Main loop: wait for multiple events
	while (TRUE)
	{
		// Wait for the overlapped I/O call to complete - forever
		if ((Event = WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents failed with error %d\n", WSAGetLastError());
			return;
		}

		// WSAEnumNetworkEvents only reports network activity and errors nominated through WSAEventSelect.
		// return value of WSAWaitMultEvt minus WSA_WAIT_EVENT_0 indicates the index of the event object whose state caused the function to return. 
		if (WSAEnumNetworkEvents(SocketArray[Event - WSA_WAIT_EVENT_0]->Socket, EventArray[Event - WSA_WAIT_EVENT_0], &NetworkEvents) == SOCKET_ERROR)
		{
			printf("WSAEnumNetworkEvents failed with error %d\n", WSAGetLastError());
			return;
		}

		// If FD_ACCEPT EVENT
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
		{
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
				break;
			}

			if ((AcceptSocket = accept(SocketArray[Event - WSA_WAIT_EVENT_0]->Socket, NULL, NULL)) == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				break;
			}

			if (EventTotal > WSA_MAXIMUM_WAIT_EVENTS)
			{
				printf("Too many connections - closing socket.\n");
				closesocket(AcceptSocket);
				break;
			}

			// Create threads to handle client here? OPTIONAL

			CreateSocketInformation(AcceptSocket);

			if (WSAEventSelect(AcceptSocket, EventArray[EventTotal - 1], FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
			{
				printf("WSAEventSelect() failed with error %d\n", WSAGetLastError());
				return;
			}

			// ==== Finally connected ====
			totalBytes = 0;
			start = std::chrono::high_resolution_clock::now();

			// Upload file option selected
			if (upload) {
				// unique filename
				char buff[20];
				auto time = std::chrono::system_clock::now();
				std::time_t time_c = std::chrono::system_clock::to_time_t(time);
				auto time_tm = *std::localtime(&time_c);
				strftime(buff, sizeof(buff), "%F-%H%M%S", &time_tm);
				std::string fileName = buff;
				out_file.open(fileName + ".txt");
			}
		}

		// Try to read and write data to and from the data buffer if read and write events occur.
		if (NetworkEvents.lNetworkEvents & FD_READ) {
			if (NetworkEvents.lNetworkEvents & FD_READ &&
				NetworkEvents.iErrorCode[FD_READ_BIT] != 0) {
				//printf("FD_READ failed with error %d\n", NetworkEvents.iErrorCode[FD_READ_BIT]);
				OutputDebugString("FD_READ FAILED");
				break;
			}

			// Points to the incoming Event's socket info
			LPSOCKET_INFORMATION SocketInfo = SocketArray[Event - WSA_WAIT_EVENT_0];

			// Counting max bytes to accept
			int left = atoi(queryPacketSizeStr);

			// Read data only if the receive buffer is empty.
			if (SocketInfo->BytesRECV == 0) {
				SocketInfo->DataBuf.buf = SocketInfo->Buffer;
				SocketInfo->DataBuf.len = atoi(queryPacketSizeStr);
				Flags = 0;
				if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags, NULL, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						printf("WSARecv() failed with error %d\n", WSAGetLastError());
						FreeSocketInformation(Event - WSA_WAIT_EVENT_0);
						return;
					}
				}
				else
				{
					char cstr[INPUT_MAX_CHAR];
					totalBytes += RecvBytes;
					SocketInfo->BytesRECV = 0;
					sprintf(cstr, "&RecvBytes : %lu TotalBytes: %lu %\n", RecvBytes, totalBytes);

					OutputDebugString(cstr);
					// Have read this much
					//SocketInfo->BytesRECV = 0;
					//SocketInfo->BytesRECV = RecvBytes;
					//SocketInfo->DataBuf.len -= RecvBytes;
					//left -= RecvBytes;
				}
			}
			//// Things to write
			//if (SocketInfo->BytesRECV > SocketInfo->BytesWRITTEN)
			//{
			//	totalBytes += countActualBytes(SocketInfo->DataBuf.buf, SocketInfo->DataBuf.len);

			//	// If we're in upload mode
			//	if (upload) {
			//		out_file << SocketInfo->DataBuf.buf;
			//	}
			//	// Push buffer pointer up
			//	SocketInfo->DataBuf.buf = SocketInfo->Buffer + SocketInfo->BytesWRITTEN;
			//	SocketInfo->DataBuf.len = SocketInfo->BytesRECV - SocketInfo->BytesWRITTEN;

			//	// CLEAR IT
			//	SocketInfo->BytesRECV = 0;
			//	SocketInfo->BytesWRITTEN = 0;
			//}
		}
		
		// Client closes port
		//if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		//{
		//	if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
		//	{
		//		printf("FD_CLOSE failed with error %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
		//		break;
		//	}
		//	end = std::chrono::high_resolution_clock::now();
		//	totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		//	// Trigger event here to print
		//	SetEvent(print_evt);

		//	printf("Client disconnect... total bytes rec'd: %d, total time: %d(ms)\n", totalBytes, totalTime);
		//	printf("Closing socket information %d\n", SocketArray[Event - WSA_WAIT_EVENT_0]->Socket);

		//	// Reset all stats
		//	totalBytes = 0;
		//	totalTime = 0;
		//	expected_packets = 0;
		//	recv_packets = 0;

		//	out_file.close();
		//	FreeSocketInformation(Event - WSA_WAIT_EVENT_0);
		//}
	} // End while loop
}

void runTcpLoop(SOCKET Listen, bool upload) {

}

void printDword(DWORD word) {

}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: setupUDPSrv
--
--    DATE : FEB 08, 2019
--
--    REVISIONS :
--    		(FEB 08, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : int setupUDPSrv()
--
--    RETURNS : int
--				0 if everything is successful
--
--    NOTES :
--			Sets up all the global variables this program needs to make it ready to accept connections from a socket,
--			using the UDP protocol
----------------------------------------------------------------------------------------------------------------------*/
int setupUDPSrv() {
	int err;
	if (serverUDPParams.portStr[0] == '\0') {
		OutputDebugString("UDP port error");
		return 2;
	}

	// check address
	if (serverUDPParams.addrStr[0] == '\0') {
		OutputDebugString("UDP addr error");
		return 4;
	}

	if (serverUDPParams.packetSizeStr[0] == '\0') {
		OutputDebugString("UDP packet size error");
		return 3;
	}
	// Startup
	if ((err = WSAStartup(0x0202, &WSAData)) != 0) //No usable DLL
	{
		exit(err);
	}
	// Create Socket
	if ((ListenSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		exit(600);
	}

	//CreateSocketInformation(ListenSocket);

	// Bind the socket
	serverUDP.sin_family = AF_INET;
	serverUDP.sin_addr.s_addr = htonl(INADDR_ANY);
	serverUDP.sin_port = 0; // set to 0 for any port?

	// Bind Listen socket to Internet Addr, basically let system auto-config
	if (bind(ListenSocket, (PSOCKADDR)&serverUDP, sizeof(serverUDP)) == SOCKET_ERROR)
	{
		printf("bind() failed with error %d\n", WSAGetLastError());
		return (601);
	}

	// Join multicast group part
	struct ip_mreq stMreq; // relocate to global?
	stMreq.imr_multiaddr.s_addr = inet_addr(serverUDPParams.addrStr);
	stMreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(ListenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) == SOCKET_ERROR) {
		printf("setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %d\n",
			serverUDPParams.addrStr, WSAGetLastError());
	}

	lTTL = 1; // default
	if (setsockopt(ListenSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&lTTL, sizeof(lTTL)) == SOCKET_ERROR) {
		printf("setsockopt() IP_MULTICAST_TTL failed, Err: %d\n",
			WSAGetLastError());
	}

	// doesn't send to itself
	BOOL fFlag = FALSE;
	if (setsockopt(ListenSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&fFlag, sizeof(fFlag)) == SOCKET_ERROR) {
		printf("setsockopt() IP_MULTICAST_LOOP failed, Err: %d\n",
			WSAGetLastError());
	}

	clientUDP.sin_family = AF_INET;
	clientUDP.sin_addr.s_addr = inet_addr(serverUDPParams.addrStr);
	clientUDP.sin_port = htons(atoi(serverUDPParams.portStr));

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: runUdpLoop
--
--    DATE : FEB 08, 2019
--
--    REVISIONS :
--    		(FEB 08, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : void runUdpLoop(SOCKET Listen, bool upload)
--			SOCKET Listen:		The socket which this function will setup and listen on
--			bool upload:		Whether or not to have the listener server save the file
--
--    RETURNS : void
--
--    NOTES :
--			Runs the loop to listen onto the UDP port specified by the windows GUI, continues to run until program
--			ends, only 1 client allowed at any given moment.
--			Exits the entire program if any of the process in setting up is unsuccessful
----------------------------------------------------------------------------------------------------------------------*/
int runUdpLoop(SOCKET Listen, bool upload) {
	char test[128]{ "#TSMWIN\n" };
	char buf[128]{ "message sent" };

	while (TRUE) {
		if (sendto(Listen, (char *)&test, sizeof(test), 0, (struct sockaddr*)&clientUDP, sizeof(clientUDP)) < 0) {
			printf("sendto() failed, Error: %d\n", WSAGetLastError());
			return 1;
		}
		else {
			char r[1]{ '\r' };
			printScreen(cmdhwnd, buf);
			printScreen(cmdhwnd, r);
		}

		/* Wait for the specified interval */
		Sleep(10 * 1000);
	}
	closesocket(Listen);

	/* Tell WinSock we're leaving */
	WSACleanup();

	return 0;
	
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: printScreen
--
--    DATE : JAN 17, 2019
--
--    REVISIONS :
--    		(JAN 17, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : void printScreen(HWND hwnd, char *buffer)
--			HANDLE hwnd:		HANDLE to the window to be printed to
--			char * buffer:		A pointer to array of chars to print to the screen
--
--    RETURNS : void
--
--    NOTES :
		  --Call this to print text to the window
---------------------------------------------------------------------------------------------------------------------*/
void printScreen(HWND hwnd, char *buffer) {
	HDC textScreen = GetDC(hwnd);
	SIZE size;
	GetTextMetrics(textScreen, &tm);        // get text metrics 

	if (*buffer == '\r')
	{
		yPosition = yPosition + tm.tmHeight + tm.tmExternalLeading;
		xPosition = 0;
		return;
	}

	GetTextExtentPoint32(textScreen, buffer, strlen(buffer), &size);

	TextOut(textScreen, xPosition, yPosition, buffer, strlen(buffer));
	xPosition = xPosition + size.cx + 1;
	ReleaseDC(hwnd, textScreen);
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: wipeScreen
--
--    DATE : JAN 17, 2019
--
--    REVISIONS :
--    		(JAN 17, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : void wipeScreen(HWND hwnd)
--			HANDLE hwnd:		HANDLE to the window to be wiped
--
--    RETURNS : void
--
--    NOTES :
		  --Call this to reset the main result window
---------------------------------------------------------------------------------------------------------------------*/
void wipeScreen(HWND hwnd) {
	xPosition = 0;
	yPosition = 0;
	HDC textScreen = GetDC(hwnd);
	HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
	Rectangle(textScreen, 1, 1, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	LPSTR lspszCmdParam, int nCmdShow)
{
	//HWND connecthwnd;
	MSG Msg;
	WNDCLASSEX Wcl;

	Wcl.cbSize = sizeof(WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); // large icon 
	Wcl.hIconSm = NULL; // use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);  // cursor style

	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); //white background
	Wcl.lpszClassName = CmdModName;

	Wcl.lpszMenuName = TEXT("COMMENU"); // The menu Class
	Wcl.cbClsExtra = 0;      // no extra memory needed
	Wcl.cbWndExtra = 0;

	if (!RegisterClassEx(&Wcl))
		return 0;

	cmdhwnd = CreateWindow(CmdModName, CmdModName, WS_OVERLAPPEDWINDOW, 10, 10,
		600, 400, NULL, NULL, hInst, NULL);
	ShowWindow(cmdhwnd, nCmdShow);	//Show the first window - our connect module with nothing
	UpdateWindow(cmdhwnd);

	// Create print event here
	print_evt = CreateEventA(NULL, FALSE, FALSE, TEXT("PrintEvent"));

	// Message loop
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case ID_SRV_START:
			clearInputs(&serverTCPParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_QUERYBOX_SRV), hwnd, HandleTCPSrvSetup);
			if (setupTCPSrv() == 0) {
				h_thread_srv = CreateThread(NULL, 0, runTCPthread, (LPVOID)false, 0, &thread_srv_id);
				h_thread_print = CreateThread(NULL, 0, printTCPthread, (LPVOID)hwnd, 0, &thread_print_id);
			}
			break;
		case ID_SRV_MULTICAST:
			clearInputs(&serverUDPParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_QUERYBOX_SRV_MULTICAST), hwnd, HandleMulticastSetup);
			if (setupUDPSrv() == 0) { // valid inputs
				h_thread_srv = CreateThread(NULL, 0, runUDPthread, (LPVOID)false, 0, &thread_srv_id);
				h_thread_print = CreateThread(NULL, 0, printUDPthread, (LPVOID)hwnd, 0, &thread_print_id);
			}
			break;
		case ID_CLN_REQFILE:
			clearInputs(&clientTgtParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_CLN_QUERY_FILE), hwnd, HandleClnQuery);
			if (setupTCPCln(&clientTgtParams, &ClientSocket, &WSAData, &serverTCP) == 0) {
				OutputDebugString("TCPCLNSetup ok\n");
				requestTCPFile(&ClientSocket, &serverTCP, clientTgtParams.reqFilename);
			}
			OutputDebugString("REQFILE\n");
			break;
		case ID_CLN_JOINSTREAM:
			clearInputs(&clientUDPParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_CLN_JOINBROADCAST), hwnd, HandleClnJoin);
			if (setupUDPCln(&clientUDPParams, &ClientSocket, &WSAData) == 0) {
				OutputDebugString("ID_CLN_JOINSTREAM\n");
				joiningStream(&clientUDPParams, &ClientSocket);
			}
			break;
		}
		break;
	case WM_DESTROY:	// Terminate program
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: HandleTCPSrvSetup
--
--    DATE : JAN 17, 2019
--
--    REVISIONS :
--    		(JAN 17, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
--			HANDLE hDlg:		DialogBox handle
--			UINT message		WM to trigger when DialogBox is terminated
--			WPARAM wParam		Additional message-specific information.
--			LPARAM lParam		Additional message-specific information.
--
--    RETURNS : INT_PTR
--			Typically, the dialog box procedure should return TRUE if it processed the message, and FALSE if it did not. 
--			If the dialog box procedure returns FALSE, the dialog manager performs the default dialog operation in 
--			response to the message.
--
--    NOTES :
--			Handles user input as a query, once user has inputted, a custom window message will be signalled
--			The user input will be saved to a global query buffer, then will be processed
--			This is a custom DialogProc function
----------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Variables
	HWND idd_packetsize;
	const int packetSizes[] = { 1024, 4096, 20000, 60000 };

	// Reset global queryResult

	// Init error variable
	unsigned int err = 0;

	idd_packetsize = GetDlgItem(hDlg, IDD_PACKETSIZE);

	switch (message) {
	case WM_INITDIALOG:
		for (int Count = 0; Count < sizeof(packetSizes)/sizeof(const int); Count++)
		{
			SendMessage(idd_packetsize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR)std::to_string(packetSizes[Count]).c_str()));
		}
		SendMessage(idd_packetsize, CB_SETCURSEL, 0, 0);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			MessageBox(NULL, "pressed cancel", "all not ok", MB_OK);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR) FALSE;
		}
		if (LOWORD(wParam) == IDOK) {
			/*Get Server port*/
			err = GetDlgItemText(hDlg, IDT_PORT, serverTCPParams.portStr, sizeof(serverTCPParams.portStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get Packet size*/
			err = GetDlgItemText(hDlg, IDD_PACKETSIZE, serverTCPParams.packetSizeStr, sizeof(serverTCPParams.packetSizeStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK HandleMulticastSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	unsigned int err = 0;
	switch (message) {
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			MessageBox(NULL, "Pressed cancel", "all not ok", MB_OK);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)FALSE;
		}
		if (LOWORD(wParam) == IDOK) {
			/* Get Address*/
			err = GetDlgItemText(hDlg, IDT_MULTICAST_ADDRESS, serverUDPParams.addrStr, sizeof(serverUDPParams.addrStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get Server port*/
			err = GetDlgItemText(hDlg, IDT_MULTICAST_PORT, serverUDPParams.portStr, sizeof(serverUDPParams.portStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get Packet size*/
			err = GetDlgItemText(hDlg, IDT_MULTICAST_PACKETSIZE, serverUDPParams.packetSizeStr, sizeof(serverUDPParams.packetSizeStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK HandleClnQuery(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	// Variables
	HWND idd_packetsize;
	const int packetSizes[] = { 1024, 4096, 20000, 60000 };
	// Init error variable
	unsigned int err = 0;

	idd_packetsize = GetDlgItem(hDlg, IDD_CLN_PACKETSIZE);
	switch (message) {
	case WM_INITDIALOG:
		for (int Count = 0; Count < sizeof(packetSizes) / sizeof(const int); Count++)
		{
			SendMessage(idd_packetsize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR)std::to_string(packetSizes[Count]).c_str()));
		}
		SendMessage(idd_packetsize, CB_SETCURSEL, 0, 0);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			MessageBox(NULL, "pressed cancel", "cancelled", MB_OK);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)FALSE;
		}
		if (LOWORD(wParam) == IDOK) {
			/*Get Server name as a string*/
			err = GetDlgItemText(hDlg, IDT_CLN_TGTADDR, clientTgtParams.addrStr, sizeof(clientTgtParams.addrStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get Server port*/
			err = GetDlgItemText(hDlg, IDT_CLN_TGT_PORT, clientTgtParams.portStr, sizeof(clientTgtParams.portStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get Packet size*/
			err = GetDlgItemText(hDlg, IDD_CLN_PACKETSIZE, clientTgtParams.packetSizeStr, sizeof(clientTgtParams.packetSizeStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get req Filename*/
			err = GetDlgItemText(hDlg, IDT_REQFILENAME, clientTgtParams.reqFilename, sizeof(clientTgtParams.reqFilename));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get if stream mode*/
			if (IsDlgButtonChecked(hDlg, IDC_STREAM) == BST_CHECKED) {
				// Checked
				clientTgtParams.stream = true;
			}
			else {
				// Not checked
				clientTgtParams.stream = false;
			};
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK HandleClnJoin(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	// Init error variable
	unsigned int err = 0;
	switch (message) {
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			MessageBox(NULL, "Pressed cancel", "cancelled", MB_OK);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)FALSE;
		}
		if (LOWORD(wParam) == IDOK) {
			/*Get Server name as a string*/
			err = GetDlgItemText(hDlg, IDT_CLN_TGTADDR, clientUDPParams.addrStr, sizeof(clientUDPParams.addrStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			/*Get Server port*/
			err = GetDlgItemText(hDlg, IDT_CLN_TGT_PORT, clientUDPParams.portStr, sizeof(clientUDPParams.portStr));
			if (err == 0) {
				EndDialog(hDlg, LOWORD(wParam));
				MessageBoxA(hDlg, MSG_INPUT_ERR_NOINPUT, LABEL_INPUT_ERR, MB_OK);
				break;
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: runTCPthread
--
--    DATE : FEB 12, 2019
--
--    REVISIONS :
--    		(FEB 12, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : DWORD WINAPI runTCPthread(LPVOID upload)
--			VPVOID upload:		(BOOL) Whether or not to have the listener server save the file
--
--    RETURNS : DWORD
--			200 on completion
--
--    NOTES :
--			Thread function to run the TCP listening server
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI runTCPthread(LPVOID upload) {
	while (1) {
		// Listen
		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			OutputDebugString(convertErrString("listen() failed with error", WSAGetLastError()));
			return 403;
		}

		// Keep listening for Accepts
		if ((AcceptSocket = accept(ListenSocket, NULL, NULL)) == INVALID_SOCKET) {
			OutputDebugString("Invalid socket");
			return 404;
		}
		else {
			OutputDebugString("Accepted\n");
			char buf[128]{"accepted client"};
			char r[1]{ '\r' };
			printScreen(cmdhwnd, buf);
			printScreen(cmdhwnd, r);
			//CreateThread(NULL, 0, runTCPthread, (LPVOID)false, 0, &thread_srv_id);
			if ((h_thread_accept = CreateThread(NULL, 0, runAcceptThread, (LPVOID)AcceptSocket, 0, &thread_accept_id)) == NULL) {
				return 405;
			}
		}
	}
	return 200;
}

// SRV: For TCP transfer of files
DWORD WINAPI runAcceptThread(LPVOID acceptSocket) {
	OutputDebugString("Running Accept thread\n");
	// Thread variables

	// Current socket information
	DWORD Flags = 0;
	LPSOCKET_INFORMATION SocketInfo;
	if ((SocketInfo = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
		OutputDebugString("GlobalAlloc() failed\n");
		return 1;
	}
	SocketInfo->totalBytesTransferred = 0;
	// Wait for Client
	// SocketInfo population
	SocketInfo->Buffer = (char *)malloc(DATA_BUF_SIZE);
	memset(SocketInfo->Buffer, 0, sizeof(SocketInfo->Buffer));
	SocketInfo->DataBuf.len = DATA_BUF_SIZE;
	SocketInfo->DataBuf.buf = SocketInfo->Buffer;
	SocketInfo->Socket = (SOCKET)acceptSocket;
	ZeroMemory(&(SocketInfo->Overlapped), sizeof(WSAOVERLAPPED));
	SocketInfo->BytesRECV = 0;
	SocketInfo->BytesWRITTEN = 0;
	SocketInfo->Overlapped.hEvent = SocketInfo;
	Flags = 0;
	// Supposedly WFME, but we woke this thread?

	if (WSARecv((SOCKET)acceptSocket, &(SocketInfo->DataBuf), 1, NULL, &Flags, &(SocketInfo->Overlapped), recvFileReqCallback) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			OutputDebugString("PostSleep WSA Fail, client prob disconnected\n");
			char cstr[DATA_BUF_SIZE];
			char cr[1]{ '\r' };
			sprintf(cstr, "TotalBytes recv'd: %d\n", SocketInfo->totalBytesTransferred);
			printScreen(cmdhwnd, cstr);
			printScreen(cmdhwnd, cr);
			OutputDebugString(cstr);
			GlobalFree(SocketInfo);
			return 1;
		}
	}
	SleepEx(INFINITE, TRUE);
	
	// Get Filename
	FILE *fptr;
	errno_t err;
	char filename[DATA_BUF_SIZE];
	char relativePath[DATA_BUF_SIZE];
	strcpy(filename, SocketInfo->Buffer);
	_getcwd(relativePath, DATA_BUF_SIZE);
	std::string fullPath = relativePath;
	fullPath.append("\\");
	fullPath.append(filename);
	
	OutputDebugString(fullPath.c_str());
	OutputDebugString("\n");
	if ((err = fopen_s(&fptr, fullPath.c_str(), "r") != 0)) {
		OutputDebugString("file open error");
		exit(404);
	}

	// Reset the buffer
	memset(SocketInfo->Buffer, 0, sizeof(SocketInfo->Buffer));

	// Send loop, probably
	char ch;
	int bufInd = 0;
	while ((ch = fgetc(fptr)) != EOF) {
		if (bufInd < DATA_BUF_SIZE - 1) {
			SocketInfo->Buffer[bufInd] = ch;
			bufInd++;
		} else {
			// Append last letter
			SocketInfo->Buffer[bufInd] = ch;
			// Buffer filled, gotta send whole buffer
			while (SocketInfo->BytesWRITTEN < DATA_BUF_SIZE) {
				// Not all sent
				if (WSASend((SOCKET)acceptSocket, &(SocketInfo->DataBuf), 1, NULL, Flags, &(SocketInfo->Overlapped), srvSentFileCallback) == SOCKET_ERROR) {
					int last_err = WSAGetLastError();
					if (!(last_err == WSA_IO_PENDING || last_err == WSAEWOULDBLOCK))
					{
						printScreen(cmdhwnd, convertErrString("WSASend failed, not IO_PENDING or WOUDLBLOCK", WSAGetLastError()));
						return 1;
					}
				}
				SleepEx(INFINITE, TRUE);
			}
			// All sent
			memset(SocketInfo->Buffer, 0, sizeof(SocketInfo->Buffer));
			SocketInfo->BytesWRITTEN = 0;
			bufInd = 0;
		}
	}
	// Show progress of final
	char pstr[128];
	char cr[1]{ '\r' };
	sprintf(pstr, "Last buffer index: %d\n", bufInd);
	SocketInfo->DataBuf.len = bufInd;
	printScreen(cmdhwnd, pstr);
	printScreen(cmdhwnd, cr);

	//Send last thing
	if (WSASend((SOCKET)acceptSocket, &(SocketInfo->DataBuf), 1, NULL, Flags, &(SocketInfo->Overlapped), srvSentFileCallback) == SOCKET_ERROR)
	{
		int last_err = WSAGetLastError();
		if (!(last_err == WSA_IO_PENDING || last_err == WSAEWOULDBLOCK))
		{
			OutputDebugString("WSASend failed, not WOULDBLOCK\n");
			return 1;
		}
	}
	SleepEx(INFINITE, TRUE);

	char cstr[DATA_BUF_SIZE];
	sprintf(cstr, "Total Bytes Sent'd: %d\n", SocketInfo->totalBytesTransferred);
	OutputDebugString(cstr);

	closesocket(SocketInfo->Socket);
	GlobalFree(SocketInfo);
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: printTCPthread
--
--    DATE : FEB 12, 2019
--
--    REVISIONS :
--    		(FEB 12, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : DWORD WINAPI printTCPthread(LPVOID hwnd) 
--			VPVOID hwnd:		The window handle to which this thread should print to
--
--    RETURNS : DWORD
--			200 on completion
--
--    NOTES :
--			Thread function to run the TCP listening server
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI printTCPthread(LPVOID hwnd) {
	while (1) {
		WaitForSingleObject(print_evt, INFINITE);
		char cstr[INPUT_MAX_CHAR];
		char cr[INPUT_MAX_CHAR]{'\r'};
		sprintf(cstr, "Client disconnect... total bytes rec'd: %d, total time: %d(ms)\n", totalBytes, totalTime);
		printScreen((HWND)hwnd, cstr);
		printScreen((HWND)hwnd, cr);
	}
	return 200;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: runUDPthread
--
--    DATE : FEB 12, 2019
--
--    REVISIONS :
--    		(FEB 12, 2019): Created
--			(MAR 19, 2019): Edited to fit comm audio server
--
--    DESIGNER : Jacky Li, Alexander Song
--
--    PROGRAMMER : Alexander Song
--
--    INTERFACE : DWORD WINAPI runUDPthread(LPVOID upload)
--			VPVOID upload:		(BOOL) Whether or not to have the listener server save the file
--
--    RETURNS : DWORD
--			200 on completion
--
--    NOTES :
--			Thread function to run the UDP listening server
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI runUDPthread(LPVOID upload) {
	while (1) {
		if (runUdpLoop(ListenSocket, (BOOL)upload) == 1) {
			OutputDebugString("runUDPthread error\n");
			break;
		}
	}
	return 200;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: printUDPthread
--
--    DATE : FEB 12, 2019
--
--    REVISIONS :
--    		(FEB 12, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li
--
--    INTERFACE : DWORD WINAPI printUDPthread(LPVOID hwnd)
--			VPVOID hwnd:		The window handle to which this thread should print to
--
--    RETURNS : DWORD
--			200 on completion
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI printUDPthread(LPVOID hwnd) {
	while (1) {
		WaitForSingleObject(print_evt, INFINITE);
		char cstr[INPUT_MAX_CHAR];
		char cr[INPUT_MAX_CHAR]{ '\r' };
		//printf("Client EOT... total bytes rec'd: %d, total time: %d(ms), data packets: %d/%d\n", totalBytes, totalTime, recv_packets, expected_packets);
		sprintf(cstr, "Client disconnect... total bytes rec'd: %d, total time: %d(ms) data packets: %d/%d\n", totalBytes, totalTime, recv_packets, expected_packets);
		printScreen((HWND)hwnd, cstr);
		printScreen((HWND)hwnd, cr);
	}
	return 200;
}

// Create a socket information struct based on the passed in SOCKET, inits SI values, and shoves it into a global array of sockets
BOOL CreateSocketInformation(SOCKET s)
{
	LPSOCKET_INFORMATION SI;

	if ((EventArray[EventTotal] = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
		return FALSE;
	}

	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
	{
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return FALSE;
	}

	// Prepare SocketInfo structure for use.
	SI->Socket = s;
	SI->BytesWRITTEN = 0;
	SI->BytesRECV = 0;
	SocketArray[EventTotal] = SI;

	// Setup Buffer size
	//SI->Buffer = (char *)malloc(atoi(queryPacketSizeStr) * sizeof(char));
	//memset(SI->Buffer, '\0', atoi(queryPacketSizeStr));

	EventTotal++;

	return(TRUE);
}

// Frees socket information
void FreeSocketInformation(DWORD Event)
{
	LPSOCKET_INFORMATION SI = SocketArray[Event];
	DWORD i;

	closesocket(SI->Socket);

	GlobalFree(SI);

	WSACloseEvent(EventArray[Event]);

	// Squash the socket and event arrays

	for (i = Event; i < EventTotal; i++)
	{
		EventArray[i] = EventArray[i + 1];
		SocketArray[i] = SocketArray[i + 1];
	}

	EventTotal--;
}
