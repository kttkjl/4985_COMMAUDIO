#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "User32.lib")
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
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <strsafe.h>
#include <tchar.h> 
#include "resource.h"
#include "network.h"
#include "SocketInformation.h"
#include "Callbacks.h"
#include "QueryParams.h"

#define PACKET_SIZE			8192
#define TEXT_BUF_SIZE		128

static bool discBool = true;


static HANDLE pb_print_thread;
static DWORD thread_print_id;

// Display functions
void printScreen(HWND hwnd, char *buffer);
void modPrintScreen(HWND hwnd, char *buffer, int startX);
void wipeScreen(HWND hwnd);
void printLibrary(HWND h);
void clearInputs(LPQueryParams qp);
void set_print_x(int x);
void set_print_y(int y);
void playLocalWaveFile(bool pick);
void stopPlayback();

// Custom functions
int runUdpLoop(SOCKET s, bool upload);

DWORD WINAPI printSoundProgress(LPVOID hwnd);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK HandleTCPSrvSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HandleMulticastSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HandleClnQuery(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HandleClnJoin(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Thread functions
DWORD WINAPI runTCPthread(LPVOID upload);
DWORD WINAPI runUDPthread(LPVOID upload);
DWORD WINAPI runAcceptThread(LPVOID acceptSocket);
DWORD WINAPI runUDPRecvthread(LPVOID recv);

