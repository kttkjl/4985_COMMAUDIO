#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Winmm.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <ctime>
#include <fstream>
#include <chrono>
#include "SocketInformation.h"
#include "QueryParams.h"
#include "utilities.h"
#include "Callbacks.h"
#include "command.h"

#define CHUNK_SIZE 8192
#define CHUNK_NUM 3000

int setupTCPCln(LPQueryParams, SOCKET *, WSADATA *, SOCKADDR_IN *);
int setupUDPCln(LPQueryParams, SOCKET *, WSADATA *);
int requestTCPFile(SOCKET * , SOCKADDR_IN *, const char *, HWND);
void joiningStream(LPQueryParams, SOCKET *, HWND, bool *);

static WAVEHDR* allocateBufferMemory();
static void addtoBufferAndPlay(HWAVEOUT hWaveOut, LPSTR data, int size);
static void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);

static volatile int			waveFreeBlockCount;
static int					waveCurrentBlock;
static CRITICAL_SECTION		mutex;
static WAVEHDR*				chunkBuffer;
