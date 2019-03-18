#include "network.h"

/*------------------------------------------------------------------------------------------------------------------
--    FUNCTION: resolveAsyncError
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
--    INTERFACE : void resolveAsyncError(HWND hwnd, int err_no)
--			HANDLE hwnd:				HANDLE to the owner window of the message box to be created.
--			 int err_no:				Error number obtained from WSAGetLastError();
--
--    RETURNS :	void
--
--    NOTES :
--			Creates a MessageBox of the corresponding WSAAsync error.
----------------------------------------------------------------------------------------------------------------------*/
void resolveAsyncError(HWND hwnd, int err_no) {
	switch (err_no) {
	case WSAENETDOWN:
		MessageBox(hwnd, MSG_WSAENETDOWN, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSAENOBUFS:
		MessageBox(hwnd, MSG_WSAENOBUFS, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSAEFAULT:
		MessageBox(hwnd, MSG_WSAEFAULT, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSAHOST_NOT_FOUND:
		MessageBox(hwnd, MSG_WSAHOST_NOT_FOUND, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSATRY_AGAIN:
		MessageBox(hwnd, MSG_WSATRY_AGAIN, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSANO_RECOVERY:
		MessageBox(hwnd, MSG_WSANO_RECOVERY, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSANO_DATA:
		MessageBox(hwnd, MSG_WSANO_DATA, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSANOTINITIALISED:
		MessageBox(hwnd, MSG_WSANOTINITIALISED, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSAEINPROGRESS:
		MessageBox(hwnd, MSG_WSAEINPROGRESS, LABEL_ASYNC_ERR, MB_OK);
		break;
	case WSAEWOULDBLOCK:
		MessageBox(hwnd, MSG_WSAEWOULDBLOCK, LABEL_ASYNC_ERR, MB_OK);
		break;
	default:
		MessageBox(hwnd, "UNKNOWN ERROR LMAO", LABEL_ASYNC_ERR, MB_OK);
		break;
	}
}
