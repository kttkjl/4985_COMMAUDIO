#include "Client.h"

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
		//printf("DLL not found!\n");
		exit(err);
	}
	// Create Socket
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		exit(600);
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

	// Don't deal with buffers here
	return 0;
}

int requestTCPFile(SOCKET * sock, SOCKADDR_IN * tgtAddr, const char * fileName) {
	LPSOCKET_INFORMATION SI;
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
	OutputDebugString(fileName);
	OutputDebugString("\r");
	if (WSASend(*sock, &(SI->DataBuf), 1, &(SI->BytesWRITTEN), flags, NULL, NULL)  == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			OutputDebugString("WSASend failed, not WOULDBLOCK\n");
			return 1;
		}
	}
	// Setup to save file
	std::ofstream out_file;
	char buff[20];
	auto time = std::chrono::system_clock::now();
	std::time_t time_c = std::chrono::system_clock::to_time_t(time);
	auto time_tm = *std::localtime(&time_c);
	strftime(buff, sizeof(buff), "%F-%H%M%S", &time_tm);
	std::string fn = buff;
	out_file.open(fn + ".txt");
	
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
				out_file.write(temp_buf, SI->BytesRECV);
				// Debug stats
				char cstr[256];
				sprintf(cstr, "Total Bytes Recv'd: %d\n", SI->totalBytesTransferred);
				OutputDebugString(cstr);
				// Reset
				closesocket(*sock);
				GlobalFree(SI);
				out_file.close();
				return 1;
			}
		}
		//OutputDebugString("SleepEX reached cln\n");
		SleepEx(INFINITE, TRUE);

		// Calc
		bytesToRead = bytesToRead - SI->BytesRECV;
		// Save current read buffer
		for (int i = 0; i < SI->BytesRECV; ++i) {
			packet_buf[chars_written] = SI->Buffer[i];
			chars_written++;
		}

		if (bytesToRead == 0) {
			// Full packet get, Write to file
			out_file.write(packet_buf, DATA_BUF_SIZE);
			// Reset BytesToRead and BytesRecv
			memset(SI->Buffer, '\0', DATA_BUF_SIZE);
			bytesToRead = DATA_BUF_SIZE;
			SI->BytesRECV = 0;
			SI->DataBuf.len = DATA_BUF_SIZE;
			// Temp buffer reset
			chars_written = 0;
			memset(packet_buf, '\0', DATA_BUF_SIZE);
		} else {
			// More to read, call another read again
			SI->DataBuf.len = SI->DataBuf.len - SI->BytesRECV;
			bytesToRead = SI->DataBuf.len;
		}
	}

	return 0;
}
