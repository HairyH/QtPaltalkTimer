/*********************************/
/* (c) 2020 Hairy Soft Solutions */
/*********************************/
#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#define _WIN32_WINNT _WIN32_WINNT_VISTA
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <CommCtrl.h>
#include <stdio.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <Richedit.h>

// Resource Header
#include "resource1.h"

#pragma comment(lib,"comctl32.lib")

// TODO: Change these to the App name 
char szAppName[] = "Win32DialogProject";
wchar_t wcAppName[] = L"Win32DialogProject";

#define msgba(h,x) MessageBoxA(h,x,szAppName,MB_OK)
#define msgbw(h,x) MessageBoxW(h,x,wcAppName,MB_OK)

#define MAXITEMTXT 256

#define IDT_TIMER	5555
#define IDT_TIMERU	6666
