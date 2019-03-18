#include "Callbacks.h"

void CALLBACK recvFileReqCallback(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags) {
	// Black magic fuckery here
	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)(Overlapped->hEvent);

	if (Error != 0 || BytesTransferred == 0)
	{
		// Either a bad error occurred on the socket or the socket was closed by a peer
		closesocket(SI->Socket);
		return;
	}
	// File Name assign
	char filename[DATA_BUF_SIZE];
	strcpy(filename, SI->Buffer);
	char cstr[DATA_BUF_SIZE];
	sprintf(cstr, "File name requested: %s\n", filename);
	//SI->totalBytesTransferred += countActualBytes(SI->DataBuf.buf, BytesTransferred);
	OutputDebugString(cstr);
}

// For when client gets file sent to it
void CALLBACK clnRecvFileCallback(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags) {
	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)(Overlapped->hEvent);
	if (Error != 0 || BytesTransferred == 0)
	{
		// Either a bad error occurred on the socket or the socket was closed by a peer
		closesocket(SI->Socket);
		return;
	}
	// Print stats
	char cstr[DATA_BUF_SIZE];
	SI->BytesRECV = countActualBytes(SI->DataBuf.buf, BytesTransferred);
	sprintf(cstr, "BytesRecv'd: %d\n", SI->BytesRECV);
	SI->totalBytesTransferred += SI->BytesRECV;
	//SI->totalBytesTransferred += countActualBytes(SI->DataBuf.buf, BytesTransferred);
	OutputDebugString(cstr);
}

void CALLBACK srvSentFileCallback(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags) {
	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)(Overlapped->hEvent);
	if (Error != 0 || BytesTransferred == 0)
	{
		// Either a bad error occurred on the socket or the socket was closed by a peer
		closesocket(SI->Socket);
		return;
	}
	//ZeroMemory(&Overlapped, sizeof(WSAOVERLAPPED));
	// Print stats
	char cstr[DATA_BUF_SIZE];
	sprintf(cstr, "BytesSent'd: %d\n", BytesTransferred);

	SI->BytesWRITTEN = SI->BytesWRITTEN + countActualBytes(SI->DataBuf.buf, BytesTransferred);
	SI->totalBytesTransferred += countActualBytes(SI->DataBuf.buf, BytesTransferred);
	OutputDebugString(cstr);
}
