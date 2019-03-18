#pragma once
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define DATA_BUF_SIZE 8192
#include <winsock2.h>
#include <windows.h>

typedef struct _SOCKET_INFORMATION {
	OVERLAPPED Overlapped;
	SOCKET Socket;
	CHAR * Buffer;
	WSABUF DataBuf;
	DWORD BytesWRITTEN;
	DWORD BytesRECV;
	unsigned long totalBytesTransferred;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;
