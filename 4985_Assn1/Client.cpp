#include "Client.h"

struct ip_mreq				stMreq;
SOCKADDR_IN					lclAddr, srcAddr;
LPSOCKET_INFORMATION		SI;
SOCKET *					s_sock;
extern bool clientStream;

int setupTCPCln(LPQueryParams qp, SOCKET * sock, WSADATA * wsaData, SOCKADDR_IN * tgtAddr) {
	int err;
	struct hostent *hp;
	if (qp->addrStr[0] == '\0') {
		return 1;
	}
	if (qp->portStr[0] == '\0') {
		return 2;
	}
	if (qp->packetSizeStr[0] == '\0') {
		return 3;
	}
	// Startup
	if ((err = WSAStartup(0x0202, wsaData)) != 0) //No usable DLL
	{
		OutputDebugString("DLL not found!\n");
		return err;
	}
	// Create Socket
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		return 600;
	}
	// Set sockaddr
	memset((char *)tgtAddr, 0, sizeof(SOCKADDR_IN));
	tgtAddr->sin_family = AF_INET;
	tgtAddr->sin_port = htons(atoi(qp->portStr));
	if ((hp = gethostbyname(qp->addrStr)) == NULL)
	{
		OutputDebugString("Unknown server address\n");
		return 4;
	}
	memcpy((char *)&tgtAddr->sin_addr, hp->h_addr, hp->h_length);

	return 0;
}

int requestTCPFile(SOCKET * sock, SOCKADDR_IN * tgtAddr, const char * fileName, HWND h, bool play) {
	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
		OutputDebugString("GlobalAlloc() failed\n");
		return 1;
	}
	DWORD flags = 0;
	SI->Buffer = (char *)malloc(DATA_BUF_SIZE);
	memset(SI->Buffer, 0, sizeof(SI->Buffer));
	SI->DataBuf.buf = SI->Buffer;
	strcpy(SI->DataBuf.buf, fileName);
	SI->DataBuf.len = DATA_BUF_SIZE;
	SI->Socket = *sock;
	SI->BytesRECV = 0;
	SI->BytesWRITTEN = 0;
	SI->Overlapped.hEvent = SI;

	// Connect to the server
	if (connect(*sock, (struct sockaddr *)tgtAddr, sizeof(*tgtAddr)) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			OutputDebugString(convertErrString("WSASend() failed with error", WSAGetLastError()));
			return 503;
		}
	}
	// Request file to server
	if (WSASend(*sock, &(SI->DataBuf), 1, &(SI->BytesWRITTEN), flags, NULL, NULL)  == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			OutputDebugString("WSASend failed, not WOULDBLOCK\n");
			return 1;
		}
	}
	// Setup to save file
	char buff[20];
	auto time = std::chrono::system_clock::now();
	std::time_t time_c = std::chrono::system_clock::to_time_t(time);
	auto time_tm = *std::localtime(&time_c);
	strftime(buff, sizeof(buff), "%F-%H%M%S", &time_tm);
	std::string fn = buff;
	fn += ".wav";
	FILE *fp;
	fp = fopen(fn.c_str(), "wb");
	
	// Setup Read params
	int bytesToRead = DATA_BUF_SIZE;
	char packet_buf[DATA_BUF_SIZE]{ 0 };
	int chars_written = 0;

	// Read file loop
	while (1) {
		if (WSARecv(*sock, &(SI->DataBuf), 1, NULL, &flags, &(SI->Overlapped), clnRecvFileCallback) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				// Get last thing in buffer
				char temp_buf[DATA_BUF_SIZE]{ 0 };
				memcpy(temp_buf, SI->Buffer, SI->BytesRECV);
				// Debug stats
				char cstr[256];
				sprintf(cstr, "Total Bytes Recv'd: %d\n", SI->totalBytesTransferred);
				OutputDebugString(cstr);
				// Reset
				closesocket(*sock);
				GlobalFree(SI);
				fclose(fp);

				if (play) {
					stopPlayback();
					wipeScreen(h);
					PlaySound(fn.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
					char nowplaying[TEXT_BUF_SIZE] = "Now playing: ";
					strcat(nowplaying, fn.c_str());
					printScreen(h, nowplaying);
				}

				return 1;
			} 
		}

		SleepEx(INFINITE, TRUE);

		DWORD readBytes;
		readBytes = fwrite(SI->DataBuf.buf, sizeof(char), SI->BytesRECV, fp);
		SI->BytesRECV = 0;
		memset(SI->Buffer, 0, DATA_BUF_SIZE);
	}

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: allocateBufferMemory
--
--    DATE : MAR 19, 2019
--
--
--    DESIGNER : Simon Chen
--
--    PROGRAMMER : Simon Chen
--
--    INTERFACE : WAVEHDR* allocateBufferMemory()
--
--    RETURNS : WAVEHDR*, memory allocated
--
--    NOTES : allocate memory according to CHUNK_NUM and CHUNK_SIZE
----------------------------------------------------------------------------------------------------------------------*/
WAVEHDR* allocateBufferMemory()
{
	unsigned char* buffer;
	WAVEHDR* chunks;
	DWORD bufferSize = (CHUNK_SIZE + sizeof(WAVEHDR)) * CHUNK_NUM;

	if ((buffer = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize)) == NULL) {
		//error when allocating memory
		ExitProcess(9);
	}

	chunks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * CHUNK_NUM;

	for (int i = 0; i < CHUNK_NUM; i++) {
		chunks[i].dwBufferLength = CHUNK_SIZE;
		chunks[i].lpData = (char*)buffer;
		buffer += CHUNK_SIZE;
	}

	return chunks;
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: updateChunkPosition
--
--    DATE : MAR 19, 2019
--
--
--    DESIGNER : Simon Chen
--
--    PROGRAMMER : Simon Chen
--
--    INTERFACE : void updateChunkPosition(WAVEHDR* index)
--		index: current buffer location
--
--    RETURNS : void
--
--    NOTES : update buffer location, if buffer is full wait until at least 1 buffer is free then continue
----------------------------------------------------------------------------------------------------------------------*/
void updateChunkPosition(WAVEHDR* index) {

	EnterCriticalSection(&mutex);
	chunksAvailable--;
	LeaveCriticalSection(&mutex);

	while (!chunksAvailable) {
		std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
		std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

		while (time_span.count() <= 0.01) {
			end = std::chrono::high_resolution_clock::now();
			time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
		}
	}

	chunkIndicator++;
	chunkIndicator %= CHUNK_NUM;
	index = &chunkBuffer[chunkIndicator];
	index->dwUser = 0;
}


/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: setupUDPCln
--
--    DATE : MAR 19, 2019
--
--    REVISIONS :
--    		(MAR 19, 2019): Created
--
--    DESIGNER : Alexander Song
--
--    PROGRAMMER : Alexander Song
--
--    INTERFACE : int setupUDPCln(LPQueryParams qp, SOCKET * sock, WSADATA * wsaData)
--			LPQueryParams qp:		A special struct that holds the inputted params submitted by user
--			SOCKET * sock:			The socket to receive data packets
--			WSADATA * wsaData:		Structure to contain the socket's information
--
--    RETURNS : int
--				0 if successful with no errors
--
--    NOTES :
--			This function will basically setup the connections needed for datagram transfer.
--			The address and port number entered by the user is checked for empty or not.
--			Receive socket will be setup with all settings needed for multicast function.
--			Return a number not 0 if any of the setups have an error
----------------------------------------------------------------------------------------------------------------------*/
int setupUDPCln(LPQueryParams qp, SOCKET * sock, WSADATA * wsaData)
{
	int err;
	if (qp->addrStr[0] == '\0') {
		OutputDebugString("qp address error\n");
		return 1;
	}
	if (qp->portStr[0] == '\0') {
		OutputDebugString("qp port error\n");
		return 2;
	}

	// Initialize Winsock
	if ((err = WSAStartup(0x0202, wsaData)) != 0) //No usable DLL
	{
		return err;
	}

	// Create receiver socket
	if ((*sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		MessageBox(NULL, "WSASocket error", "all not ok", MB_OK);
		OutputDebugString(convertErrString("socket() failed, Err:", WSAGetLastError()));
		WSACleanup();
		return WSAGetLastError();
	}

	BOOL fFlag = TRUE;
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag) == SOCKET_ERROR)) {
		OutputDebugString(convertErrString("setsockopt() SO_REDUSEADDR failed, Err:", WSAGetLastError()));
		return WSAGetLastError();
	}

	lclAddr.sin_family = AF_INET;
	lclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	lclAddr.sin_port = htons(atoi(qp->portStr));
	
	if (bind(*sock, (struct sockaddr*)&lclAddr, sizeof(lclAddr)) != 0) {
		OutputDebugString(convertErrString("bind() port Err:", WSAGetLastError()));
		return WSAGetLastError();
	}

	stMreq.imr_multiaddr.s_addr = inet_addr(qp->addrStr);
	stMreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) == SOCKET_ERROR) {
		OutputDebugString(convertErrString("setsockopt() IP_ADD_MEMBERSHIP address, Err:", WSAGetLastError()));
		return 800;
	}

	return 0;
}


/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: addtoBufferAndPlay
--
--    DATE : MAR 19, 2019
--
--
--    DESIGNER : Simon Chen
--
--    PROGRAMMER : Simon Chen
--
--    INTERFACE : void addtoBufferAndPlay(HWAVEOUT hWaveOut, LPSTR data, int size)
--		hWaveOut: device handle
--		LPSTR: waveform-audio data
--		int size of waveform-audio data
--
--    RETURNS : void
--
--    NOTES : add audio to buffer and queue to play
----------------------------------------------------------------------------------------------------------------------*/
void addtoBufferAndPlay(HWAVEOUT hWaveOut, LPSTR data, int size)
{
	WAVEHDR* index = &chunkBuffer[chunkIndicator];
	int remain;

	while (size > 0) {

		if (index->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, index, sizeof(WAVEHDR));
		if (size < (int)(CHUNK_SIZE - index->dwUser)) {
			memcpy(index->lpData + index->dwUser, data, size);
			index->dwUser += size;
			break;
		}
		remain = CHUNK_SIZE - index->dwUser;
		memcpy(index->lpData + index->dwUser, data, remain);
		size -= remain;
		data += remain;
		index->dwBufferLength = CHUNK_SIZE;
		waveOutPrepareHeader(hWaveOut, index, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, index, sizeof(WAVEHDR));

		updateChunkPosition(index);
	}
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: joiningStream
--
--    DATE : MAR 19, 2019
--
--    REVISIONS :
--    		(MAR 19, 2019): Created
--			(MAR 27, 2019): Play audio received (Simon)
--
--    DESIGNER : Alexander Song
--
--    PROGRAMMER : Alexander Song, Simon Chen
--
--    INTERFACE : void joiningStream(LPQueryParams qp, SOCKET * sock, HWND hwnd)
--			LPQueryParams qp:		A special struct that holds the inputted params submitted by user
--			SOCKET * sock:			The socket to receive data packets
--			HWND hwnd:				Handle for the main window
--
--    RETURNS : void
--
--    NOTES :
--			This function contains the WSARecvFrom call. The client will constantly be trying to receive
--			any incoming datagrams through overlapped and completion routine IO.
--			When a datagram is received, it will be read and play the sound in the packets.
----------------------------------------------------------------------------------------------------------------------*/
void joiningStream(LPQueryParams qp, SOCKET * sock, HWND hwnd, bool * dB)
{
	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
		OutputDebugString("GlobalAlloc() failed\n");
		exit(1);
	}

	HWAVEOUT hWaveOut;
	WAVEFORMATEX wfx;

	int err;
	DWORD flags = 0;
	s_sock = sock;
	SI->Buffer = (char *)malloc(AUD_BUF_SIZE);
	memset(SI->Buffer, 0, sizeof(SI->Buffer));
	SI->DataBuf.buf = SI->Buffer;
	SI->DataBuf.len = AUD_BUF_SIZE;
	SI->Socket = *sock;
	SI->BytesRECV = 0;
	SI->BytesWRITTEN = 0;
	SI->Overlapped.hEvent = SI;

	// Setup Read params
	int bytesToRead = AUD_BUF_SIZE;
	char packet_buf[AUD_BUF_SIZE]{ 0 };
	int chars_written = 0;


	//initialize 
	chunkBuffer = allocateBufferMemory();
	chunksAvailable = CHUNK_NUM;
	chunkIndicator = 0;
	InitializeCriticalSection(&mutex);

	//default wave header spec
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nChannels = 2;
	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) / 8 ;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)&chunksAvailable, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
		//error opening playback device
		ExitProcess(10);
	}

	while (clientStream) {
		int addr_size = sizeof(struct sockaddr_in);

		if (WSARecvFrom(*sock, &(SI->DataBuf), 1, NULL, &flags, (SOCKADDR *)& srcAddr, &addr_size, &SI->Overlapped, completeCallback) != 0) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				OutputDebugString(convertErrString("IO error, Err:", WSAGetLastError()));

				// Get last thing in buffer
				char temp_buf[AUD_BUF_SIZE]{ 0 };
				memcpy(temp_buf, SI->Buffer, SI->BytesRECV);

				// Reset
				closesocket(*sock);
				GlobalFree(SI);
				WSACleanup();
				return;
			}
		}

		SleepEx(INFINITE, TRUE);

		addtoBufferAndPlay(hWaveOut, SI->DataBuf.buf, SI->DataBuf.len);

	} // end of infinite loop

	//wait for sound to finish playing
	//while (chunksAvailable < CHUNK_NUM) {}

	waveOutPause(hWaveOut);

	//unprepare all chunks
	for (int i = 0; i < chunksAvailable; i++) {
		if (chunkBuffer[i].dwFlags & WHDR_PREPARED) {
			waveOutUnprepareHeader(hWaveOut, &chunkBuffer[i], sizeof(WAVEHDR));
		}

	}

	
	//audio clean up
	DeleteCriticalSection(&mutex);
	HeapFree(GetProcessHeap(), 0, chunkBuffer);
	waveOutClose(hWaveOut);

	stMreq.imr_multiaddr.s_addr = inet_addr(qp->addrStr);
	stMreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(*sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) == SOCKET_ERROR) {
		OutputDebugString(convertErrString("setsockopt() IP_DROP_MEMBERSHIP address failed, Err:", WSAGetLastError()));
	}

	closesocket(*sock);

	/* Tell WinSock we're leaving */
	WSACleanup();
}



/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: waveOutProc
--
--    DATE : MAR 19, 2019
--
--
--    DESIGNER : Simon Chen
--
--    PROGRAMMER : Simon Chen
--
--    INTERFACE : void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
--
--    RETURNS : void
--
--    NOTES :
--			Wavedevice callback function(runs on different thread)
--			update WAVEHDR array index
----------------------------------------------------------------------------------------------------------------------*/
static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg != WOM_DONE) {
		//device open 
		return;
	}

	EnterCriticalSection(&mutex);
	chunksAvailable++;
	LeaveCriticalSection(&mutex);
}