#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "AsyncErrorText.h"
#include "resource.h"

void resolveAsyncError(HWND hwnd, int err_no);
