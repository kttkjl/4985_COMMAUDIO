#pragma once
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
// C RunTime Header Files
#include <malloc.h>
#include <stdio.h>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include "resource.h"
#include "network.h"
#include "SocketInformation.h"
#include "Callbacks.h"
#include "QueryParams.h"

void printScreen(HWND hwnd, char *buffer);
void wipeScreen(HWND hwnd);

void clearInputs(LPQueryParams qp);

INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HandleMulticastSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HandleClnQuery(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HandleClnJoin(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
