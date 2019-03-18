#pragma once
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ctime>
#include <fstream>
#include <chrono>
#include "SocketInformation.h"
#include "QueryParams.h"
#include "utilities.h"
#include "Callbacks.h"


int setupTCPCln(LPQueryParams, SOCKET *, WSADATA *, SOCKADDR_IN *);
int requestTCPFile(SOCKET * , SOCKADDR_IN *, const char *);
