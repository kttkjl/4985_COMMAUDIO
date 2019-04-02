/*------------------------------------------------------------------------------------------------------------------
	--	SOURCE FILE : Command.cpp - An application that will receive data, and prints out the statistics for the transaction: 
	--					(1) The total bytes received from the socket
	--					(2) The total time it took to complete the entire transaction
	--					(3) Packets received/Packets expected if on UDP
	--					(4) Save the incoming data if the user chooses to do so into a .txt file
	--	
	--	PROGRAM :	4985_COMMAUDIO.exe
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
	--		void runUdpLoop(SOCKET s, bool upload);
	--		
	--		int countActualBytes(char * buf, int len);
	--	
	--		DWORD WINAPI runTCPthread(LPVOID upload);
	--		
	--		DWORD WINAPI runUDPthread(LPVOID upload);
	--		
	--	
	--		INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	--	
	--	DATE: MAR 10, 2019
	--	
	--	REVISIONS : 
	--			MAR 10, 2019: Created
	--	
	--	DESIGNER : Jacky Li, Alexander Song, Simon Chen
	--	
	--	PROGRAMMER : Jacky Li, Alexander Song
	--	
	--	NOTES :
	--		The program will listen on sockets using TCP/UDP protocol, and collect statistics on the transaction
	--		The program will continue to listen on the specified ports, until quit, or a new one has been chosen
-------------------------------------------------------------------------------------------------------------------*/

#define STRICT
#include "command.h"
#include "Client.h"

#define PACKET_SIZE 8192

static TCHAR CmdModName[] = TEXT("Team6 CommAudio");
HWND cmdhwnd; // Window handler for main window
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Query params, along with file
QueryParams serverTCPParams;
QueryParams serverUDPParams;
QueryParams clientUDPParams;
QueryParams clientTgtParams;

// Packet size
char * sbuf;
int buflen;

// Custom functions
int runUdpLoop(SOCKET s, bool upload);
void printScreen(HWND hwnd, char *buffer);

// Thread functions
DWORD WINAPI runTCPthread(LPVOID upload);
DWORD WINAPI runUDPthread(LPVOID upload);
DWORD WINAPI runAcceptThread(LPVOID acceptSocket);
DWORD WINAPI runUDPRecvthread(LPVOID recv);

// WSA Events, only need write_event for now
SOCKET				ListenSocket;
SOCKET				AcceptSocket;
SOCKET				ClientSocket;
WSADATA				WSAData;
// Resuable by both client and server
SOCKADDR_IN			serverTCP;
SOCKADDR_IN			serverUDP;
SOCKADDR_IN			clientUDP;

DWORD	thread_srv_id;
DWORD	thread_accept_id; //Throw this into array for multiple clients

HANDLE	h_thread_srv;
HANDLE  h_thread_accept; //Throw this into array for multiple clients

DWORD Flags;
DWORD RecvBytes;

// Text metrics for printing to screen
TEXTMETRIC tm;
int xPosition;
int yPosition;

u_long lTTL;
bool doneplaying = false;
std::string library[128];
int libindex = -1;

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
		OutputDebugString("TCP port empty\n");
		return 2;
	}
	if (serverTCPParams.packetSizeStr[0] == '\0') {
		OutputDebugString("Packet size empty\n");
		return 3;
	}
	// Startup
	if ((err = WSAStartup(0x0202, &WSAData)) != 0) //No usable DLL
	{
		OutputDebugString(convertErrString("WSAStartup error:", err));
		return (err);
	}
	// Create Socket
	if ((ListenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		OutputDebugString(convertErrString("ListenSocket error:", WSAGetLastError()));
		return (600);
	}

	// Bind socket
	serverTCP.sin_family = AF_INET;
	serverTCP.sin_addr.s_addr = htonl(INADDR_ANY);
	serverTCP.sin_port = htons(atoi(serverTCPParams.portStr));
	if (bind(ListenSocket, (PSOCKADDR)&serverTCP, sizeof(serverTCP)) == SOCKET_ERROR)
	{
		OutputDebugString(convertErrString("bind() failed with error\n", WSAGetLastError()));
		return 601;
	}


	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: setupUDPSrv
--
--    DATE : MAR 18, 2019
--
--    REVISIONS :
--    		(MAR 18, 2019): Created
--
--    DESIGNER : Alexander Song
--
--    PROGRAMMER : Alexander Song
--
--    INTERFACE : int setupUDPSrv()
--
--    RETURNS : int
--				0 if everything is successful
--
--    NOTES :
--			Sets up all the global variables this program needs to make it ready to accept connections from a socket,
--			using the UDP protocol for a multicast server
----------------------------------------------------------------------------------------------------------------------*/
int setupUDPSrv() {
	int err;
	// Check port
	if (serverUDPParams.portStr[0] == '\0') {
		OutputDebugString("UDP port empty\n");
		return 2;
	}

	// check address
	if (serverUDPParams.addrStr[0] == '\0') {
		OutputDebugString("Multicast addr empty\n");
		return 4;
	}

	// Startup
	if ((err = WSAStartup(0x0202, &WSAData)) != 0) //No usable DLL
	{
		OutputDebugString(convertErrString("WSAStartup error:", err));
		return (err);
	}
	// Create Socket
	if ((ListenSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		OutputDebugString(convertErrString("ListenSocket error:", WSAGetLastError()));
		return 600;
	}

	// Bind the socket
	serverUDP.sin_family = AF_INET;
	serverUDP.sin_addr.s_addr = htonl(INADDR_ANY);
	serverUDP.sin_port = 0;

	// Bind Listen socket to Internet Addr, basically let system auto-config
	if (bind(ListenSocket, (PSOCKADDR)&serverUDP, sizeof(serverUDP)) == SOCKET_ERROR)
	{
		OutputDebugString(convertErrString("bind() failed with error\n", WSAGetLastError()));
		return (601);
	}

	// Join multicast group part
	struct ip_mreq stMreq; // relocate to global?
	stMreq.imr_multiaddr.s_addr = inet_addr(serverUDPParams.addrStr);
	stMreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(ListenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) == SOCKET_ERROR) {
		OutputDebugString(convertErrString("setsockopt() IP_ADD_MEMBERSHIP address:\n", WSAGetLastError()));
		return 800;
	}

	lTTL = 1; // default time to live
	if (setsockopt(ListenSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&lTTL, sizeof(lTTL)) == SOCKET_ERROR) {
		OutputDebugString(convertErrString("setsockopt() IP_MULTICAST_TTL failed, Err:", WSAGetLastError()));
	}

	// Doesn't send to itself in the multicast
	BOOL fFlag = FALSE;
	if (setsockopt(ListenSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&fFlag, sizeof(fFlag)) == SOCKET_ERROR) {
		OutputDebugString(convertErrString("setsockopt() IP_MULTICAST_LOOP failed, Err:", WSAGetLastError()));
	}

	clientUDP.sin_family = AF_INET;
	clientUDP.sin_addr.s_addr = inet_addr(serverUDPParams.addrStr);
	clientUDP.sin_port = htons(atoi(serverUDPParams.portStr));

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: runUdpLoop
--
--    DATE : MAR 18, 2019
--
--    REVISIONS :
--    		(MAR 18, 2019): Created
--
--    DESIGNER : Alexander Song
--
--    PROGRAMMER : Alexander Song
--
--    INTERFACE : void runUdpLoop(SOCKET Listen, bool upload)
--			SOCKET Listen:		The socket which this function will setup and listen on
--			bool upload:		Whether or not to have the listener server save the file
--
--    RETURNS : void
--
--    NOTES :
--			Runs the loop to listen onto the UDP port specified by the windows GUI, continues to run until program
--			ends.
--			Exits the entire program if any of the process in setting up is unsuccessful
----------------------------------------------------------------------------------------------------------------------*/
int runUdpLoop(SOCKET Listen, bool upload) {
	// Socket info setup
	LPSOCKET_INFORMATION SI;
	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
		OutputDebugString("GlobalAlloc() failed\n");
		return 599;
	}

	DWORD Flags = 0;
	int counter = 0;
	int addr_size = sizeof(struct sockaddr_in);
	char buffer[AUD_BUF_SIZE]; // buffer to read .wav file

	SI->DataBuf.len = PACKET_SIZE;

	FILE *fp;
	char songname[128]{ "song.wav" };
	//char songname[128]{ "./Library/Faded.wav" };
	char nowplaying[128]{ "Now Playing: " };
	char errormsg[128]{ "Broadcast error" };
	char broadcastdone[128]{ "Broadcast ended" };
	fp = fopen(songname, "rb");

	discBool = false;

	while (TRUE) {
		if (discBool) {
			OutputDebugString("Disconnect clicked\n");
			break;
		}

		DWORD readBytes;
		readBytes = fread(buffer, sizeof(char), sizeof(buffer), fp);

		if (readBytes == 0) {
			OutputDebugString("Done sending\n");
			break;
		}
				
		//empty unloaded space in buffer if buffer isn't full
		if (readBytes < sizeof(buffer)) {

			memset(buffer + readBytes, 0, sizeof(buffer) - readBytes);
		}

		SI->DataBuf.buf = &buffer[0];

		if (WSASendTo(Listen, &(SI->DataBuf), 1, &(SI->BytesWRITTEN), Flags, (SOCKADDR *) & clientUDP, addr_size, &(SI->Overlapped), NULL) < 0) {
			OutputDebugString(convertErrString("WSASendTo() failed, Error: %d\n", WSAGetLastError()));

			return 1;
		}
		else {
			++counter;
			if (counter == 1) {
				set_print_x(380);
				set_print_y(200);
				printScreen(cmdhwnd, nowplaying);
				printScreen(cmdhwnd, songname);
			}
		}

		//throttle send frequency to prevent client buffer overflow
		//Sleep(20);
		auto start = std::clock();
		while ((std::clock() - start) != 20 && !discBool) { }
	}

	OutputDebugString("Leaving Multicast server\n");
	closesocket(Listen);

	wipeScreen(cmdhwnd);
	printScreen(cmdhwnd, broadcastdone);

	/* Tell WinSock we're leaving */
	WSACleanup();
	GlobalFree(SI);
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
--			(MAR 23, 2019): Slight modification
--    		(JAN 17, 2019): Created
--
--    DESIGNER : Jacky Li
--
--    PROGRAMMER : Jacky Li, Alexander Song
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
	//HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
	Rectangle(textScreen, -1, -1, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
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
		800, 600, NULL, NULL, hInst, NULL);
	ShowWindow(cmdhwnd, nCmdShow);	//Show the first window - our connect module with nothing
	UpdateWindow(cmdhwnd);

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
		switch(LOWORD(wParam)) {
		case ID_SRV_START:
			wipeScreen(cmdhwnd);
			clearInputs(&serverTCPParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_QUERYBOX_SRV), hwnd, HandleTCPSrvSetup);
			if (setupTCPSrv() == 0) {
				h_thread_srv = CreateThread(NULL, 0, runTCPthread, (LPVOID)false, 0, &thread_srv_id);
			}
			break;
		case ID_SRV_MULTICAST:
			wipeScreen(cmdhwnd);
			clearInputs(&serverUDPParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_QUERYBOX_SRV_MULTICAST), hwnd, HandleMulticastSetup);
			if (setupUDPSrv() == 0) { // valid inputs
				h_thread_srv = CreateThread(NULL, 0, runUDPthread, (LPVOID)false, 0, &thread_srv_id);
			}
			break;
		case ID_CLN_REQFILE:
			wipeScreen(cmdhwnd);
			clearInputs(&clientTgtParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_CLN_QUERY_FILE), hwnd, HandleClnQuery);
			if (setupTCPCln(&clientTgtParams, &ClientSocket, &WSAData, &serverTCP) == 0) {
				requestTCPFile(&ClientSocket, &serverTCP, clientTgtParams.reqFilename);
			}
			break;
		case ID_CLN_JOINSTREAM:
			wipeScreen(cmdhwnd);
			clearInputs(&clientUDPParams);
			DialogBox(NULL, MAKEINTRESOURCE(IDD_CLN_JOINBROADCAST), hwnd, HandleClnJoin);
			if (setupUDPCln(&clientUDPParams, &ClientSocket, &WSAData) == 0) {
				h_thread_accept = CreateThread(NULL, 0, runUDPRecvthread, (LPVOID)cmdhwnd, 0, NULL);
			}
			break;
		case ID_GEN_DISCONNECT:
			discBool = true;
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

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: HandleMulticastSrvSetup
--
--    DATE : MAR 17, 2019
--
--    REVISIONS :
--    		(MAR 17, 2019): Created
--
--    DESIGNER : Jacky Li, Alexander Song
--
--    PROGRAMMER : Alexander Song
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

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: HandleClnJoin
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
--    INTERFACE : INT_PTR CALLBACK HandleClnJoin(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
INT_PTR CALLBACK HandleClnJoin(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	// Init error variable
	unsigned int err = 0;
	switch (message) {
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			MessageBox(NULL, "Pressed cancel", "Cancelled", MB_OK);
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
			char buf[128]{"Accepted client"};
			char r[1]{ '\r' };
			printScreen(cmdhwnd, buf);
			printScreen(cmdhwnd, r);
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
	//SocketInfo->Buffer = (char *)malloc(DATA_BUF_SIZE);
	//memset(SocketInfo->Buffer, 0, sizeof(char) * DATA_BUF_SIZE);
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
	if ((err = fopen_s(&fptr, fullPath.c_str(), "rb") != 0)) {
		OutputDebugString("file open error");
		exit(404);
	}

	// Reset the buffer
	//memset(SocketInfo->Buffer, 0, DATA_BUF_SIZE);

	// Send loop, probably
	char ch;
	int bufInd = 0;
	DWORD readBytes;
	char sendBuffer[PACKET_SIZE];
	while ((readBytes = fread(sendBuffer, sizeof(char), sizeof(sendBuffer), fptr)) >0) {

		SocketInfo->DataBuf.buf = &sendBuffer[0];
			SocketInfo->DataBuf.len = PACKET_SIZE;
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
	// Show progress of final
	char pstr[128];
	char cr[1]{ '\r' };
	char sentmsg[128]{ " sent to client" };
	sprintf(pstr, "Last buffer index: %d\n", bufInd);
	SocketInfo->DataBuf.len = bufInd;

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

	printScreen(cmdhwnd, filename);
	printScreen(cmdhwnd, cr);

	closesocket(SocketInfo->Socket);
	GlobalFree(SocketInfo);
	return 0;
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
--			LPVOID upload:		(BOOL) Whether or not to have the listener server save the file
--
--    RETURNS : DWORD
--			200 on completion
--
--    NOTES :
--			Thread function to run the UDP listening server
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI runUDPthread(LPVOID upload) {
	printLibrary(cmdhwnd);
	while (1) {
		if (runUdpLoop(ListenSocket, (BOOL)upload) == 1) {
			OutputDebugString("runUDPthread exit\n");
			break;
		}
		/*else if (check if there are other songs){

		}*/
	}
	doneplaying = true;
	return 200;
}

DWORD WINAPI runUDPRecvthread(LPVOID recv) {
	joiningStream(&clientUDPParams, &ClientSocket, (HWND) recv, &discBool);
	OutputDebugString("Finished runUDPRecvthread\n");
	return 200;
}

DWORD WINAPI printSoundProgress(LPVOID hwnd) {
	int counter = 0;
	char dot[2] = ".";
	char listening_msg[128] = "Listening to radio";
	char done[128] = "Done broadcast";
	bool listen_bool = false;

	discBool = false;

	while (1) {
		if (discBool)
			break;

		if (counter == 5) {
			wipeScreen(cmdhwnd);
			counter = 0;
			listen_bool = false;
		}

		if (!listen_bool) {
			printScreen(cmdhwnd, listening_msg);
			listen_bool = true;
		}

		printScreen(cmdhwnd, dot);
		++counter;
		Sleep(1000);
	}
	wipeScreen(cmdhwnd);
	printScreen(cmdhwnd, done);
	return 700;
}

void printLibrary(HWND h) {
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	HDC textScreen = GetDC(h);
	char szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	char r[128] = "\r";
	char nosongs[128] = "No songs";

	Rectangle(textScreen, 5, 5, 175, 530);

	strcpy(szDir, "./");
	//strcpy(szDir, "./Library");
	strcat(szDir, "\\*");

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
		return;
	}

	// List all the files in the directory with some info about them.

	set_print_x(7);
	set_print_y(7);

	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			++libindex;
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			modPrintScreen(h, ffd.cFileName, 7);
			modPrintScreen(h, r, 7);
			std::string tmp(ffd.cFileName);
			library[libindex] = tmp;
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (libindex == -1) {
		modPrintScreen(h, nosongs, 7);
	}

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
	}

	FindClose(hFind);
	//return dwError;
}


void DisplayErrorBox(LPCSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and clean up

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

void modPrintScreen(HWND hwnd, char *buffer, int startX) {
	HDC textScreen = GetDC(hwnd);
	SIZE size;
	GetTextMetrics(textScreen, &tm);        // get text metrics 

	if (*buffer == '\r')
	{
		yPosition = yPosition + tm.tmHeight + tm.tmExternalLeading;
		set_print_x(startX);
		return;
	}

	GetTextExtentPoint32(textScreen, buffer, strlen(buffer), &size);

	TextOut(textScreen, xPosition, yPosition, buffer, strlen(buffer));
	xPosition = xPosition + size.cx + 1;
	ReleaseDC(hwnd, textScreen);
}

void set_print_x(int x) {
	xPosition = x;
}

void set_print_y(int y) {
	yPosition = y;
}
