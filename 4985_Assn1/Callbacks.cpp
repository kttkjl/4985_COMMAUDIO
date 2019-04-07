#include "Callbacks.h"

HWND hwnd;

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: set_hwnd
--
--    DATE : MAR 29, 2019
--
--    REVISIONS :
--			(MAR 29, 2019): Created
--
--    DESIGNER :	Alexander Song
--
--    PROGRAMMER :	Alexander Song
--
--    INTERFACE : void set_hwnd(HWND h)
--			HWND h:	the main window handle
--
--    RETURNS : void
--
--    NOTES :
--			Call this to set the window handle for callback functions to use.
---------------------------------------------------------------------------------------------------------------------*/
void set_hwnd(HWND h)
{
	hwnd = h;
}

void CALLBACK recvFileReqCallback(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags) {
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
	SI->BytesRECV = BytesTransferred;
	sprintf(cstr, "BytesRecv'd: %d\n", SI->BytesRECV);
	SI->totalBytesTransferred += SI->BytesRECV;
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

	// Print stats
	char cstr[DATA_BUF_SIZE];
	sprintf(cstr, "BytesSent'd: %d\n", BytesTransferred);

	SI->BytesWRITTEN = SI->BytesWRITTEN + BytesTransferred;
	SI->totalBytesTransferred +=  BytesTransferred;
	OutputDebugString(cstr);
}

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: completeCallback
--
--    DATE : MAR 17, 2019
--
--    REVISIONS :
--    		(MAR 17, 2019): Created
--			(APR 4, 2019): clean up
--
--    DESIGNER : Jacky Li, Alexander Song
--
--    PROGRAMMER : Alexander Song
--
--    INTERFACE : void CALLBACK completeCallback(DWORD dwError, DWORD cbTransferred, 
--						LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
--			DWORD dwError:						
--			DWORD cbTransferred:				
--			LPWSAOVERLAPPED lpOverlapped:		
--			DWORD dwFlags:						
--
--    RETURNS : void CALLBACK
--
--    NOTES :
--			Callback function that is used for the completion routine of WSARecvFrom
----------------------------------------------------------------------------------------------------------------------*/
void CALLBACK completeCallback(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags) {
	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)(lpOverlapped->hEvent);

	if (discBool)
		return;

	char cstr[AUD_BUF_SIZE];
	SI->BytesRECV = countActualBytes(SI->DataBuf.buf, cbTransferred);
	SI->totalBytesTransferred += SI->BytesRECV;
}
