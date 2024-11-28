// DialogueWin32AppTemplet.cpp : Defines the entry point for the application.
//

#include "main.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE	hInst = 0;
HWND		ghMain = 0;
HWND		ghClock = 0;
HWND		ghInterval = 0;
HWND		ghTitle = 0;
HWND		ghCbSend = 0;

// Paltalk Handles
HWND ghPtRoom = NULL, ghPtLv = NULL, ghPtMain = NULL;
// Font handles
HFONT ghFntClk = NULL;

// Timer related globals
SYSTEMTIME gsysTime;
int giMicTimerSeconds = 0;
int giInterval = 30;
char gszCurrentNick[MAXITEMTXT] = { '\0' };
char gszSavedNick[MAXITEMTXT] = { '\0' };
int iDrp = 0;
BOOL gbMonitor = FALSE;

//Quick Messagebox macro 
#define msga(x) msgba(ghMain,x)
// Quick Messagebox WIDE String 
#define msgw(x) msgbw(ghMain,x)

// Function prototypes
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL InitClockDis(void);
BOOL InitIntervals(void);
void GetPaltalkWindows(void);
BOOL CALLBACK EnumPaltalkWindows(HWND hWnd, LPARAM lParam);
void MicTimerStart(void);
void MicTimerReset(void);
BOOL GetMicUser(void);
void MonitorTimerTick(void);
void TimerMicTick(void);
void CopyPaste2Paltalk(char* szMsg);
void StartStopMonitoring(void);

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/// <summary>
/// This is the application main entry function
/// </summary>
/// <param name="hInstance">The handle to the current instance</param>
/// <param name="hPrevInstance">The handle to previous one</param>
/// <param name="lpCmdLine">The Command Line </param>
/// <param name="nShowCmd">The OS command to show or hide the app window</param>
/// <returns>Returns the exit code of the app</returns>
int APIENTRY WinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPSTR lpCmdLine,_In_ int nShowCmd)
{
	hInst = hInstance;
	// LoadLibrary(L"riched20.dll"); // uncomment if richedit is used 
	// TODO: Add any initializations as needed 
	return DialogBoxW(hInst, MAKEINTRESOURCE(IDD_DLGMAIN), NULL, (DLGPROC)DlgMain);
}

/// <summary>
/// Callback, the main message loop for the Application, to receive events
/// </summary>
/// <param name="hwndDlg">This is the Main Window Handle</param>
/// <param name="uMsg">OS sent Messages </param>
/// <param name="wParam">Extra data related to message</param>
/// <param name="lParam">Extra data related to message<</param>
/// <returns></returns>
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			ghMain = hwndDlg;
			ghClock = GetDlgItem(hwndDlg, IDC_EDIT_CLOCK);
			ghInterval = GetDlgItem(hwndDlg, IDC_COMBO_INTERVAL);
			ghTitle = GetDlgItem(hwndDlg, IDC_STATIC_TITLE);
			ghCbSend = GetDlgItem(hwndDlg, IDC_CHECK1);
			SendDlgItemMessageW(hwndDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			InitClockDis();
			if (!InitIntervals()) msga("Intervals Init Failed!");
		}
		break; // return TRUE; 
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, 0);
		}
		return TRUE;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				{
					DeleteObject(ghFntClk);
					EndDialog(hwndDlg, 0);
				}
				return TRUE;
				case IDOK:
				{
					 GetPaltalkWindows();
				}
				return TRUE;
				case IDC_BUTTON_START:
				{
					StartStopMonitoring();
				}
				return TRUE;
			}
		}
		case WM_TIMER:
		{
			if (wParam == IDT_TIMER)
			{
				TimerMicTick();
			}
			else if (wParam == IDT_TIMERU)
			{
				MonitorTimerTick();
			}
		}
		
		default:
			return FALSE;
	}
	
	return FALSE;
}

/// <summary>
/// Find the Running Paltalk Window 
/// Initialise Paltalk Control Handles
/// </summary>
void GetPaltalkWindows(void)
{
	char szWinText[200] = { '0' };
	char szTitle[256] = { '0' };

	// Paltalk chat room window
	ghPtRoom = NULL; ghPtLv = NULL;
	//ghPtOut = NULL; ghPtSend = NULL; ghPtSendPr = NULL;

	// Getting the chat room window handle and the main window handle
	ghPtRoom = FindWindowW(L"DlgGroupChat Window Class", 0); // this is to find nick list
	ghPtMain = FindWindowW(L"Qt5150QWindowIcon", 0);  // thi is to send text 

	if (ghPtRoom) // we got it
	{
		// Get the current thread ID and the target window's thread ID
		DWORD targetThreadId = GetWindowThreadProcessId(ghPtRoom, 0);
		DWORD currentThreadId = GetCurrentThreadId();
		// Attach the input of the current thread to the target window's thread
		AttachThreadInput(currentThreadId, targetThreadId, true);
		// Bring the window to the foreground
		bool bRet = SetForegroundWindow(ghPtMain);
		GetWindowTextA(ghPtRoom, szWinText, 200);
		// Finding the chat room window controls handles
		EnumChildWindows(ghPtRoom, EnumPaltalkWindows, 0);

		// Give some time for the window to come into focus
		Sleep(500);

		// Make the Paltalk Room window the HWND_TOPMOST 
		SetWindowPos(ghPtMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		// Set the title text to indicate which room we are timing
		sprintf_s(szTitle, 255, "Timing: %s", szWinText);
		SetWindowTextA(ghTitle, szTitle);
	}
	else
	{
		msga("No Paltalk Window Found!");
	}

}

/// <summary>
/// Enumeration Callback to Find the Nicks ListView control handle
/// </summary>
/// <param name="hWnd">The Handle of the current control</param>
/// <param name="lParam">Extra data, not used</param>
/// <returns>Return FALSE to stop looking</returns>
BOOL CALLBACK EnumPaltalkWindows(HWND hWnd, LPARAM lParam)
{
	LONG lgIdCnt;
	char szListViewClass[] = "SysHeader32";
	char szMsg[MAXITEMTXT] = { 0 };
	char szClassNameBuffer[MAXITEMTXT] = { 0 };

	lgIdCnt = GetWindowLongW(hWnd, GWL_ID);
	GetClassNameA(hWnd, szClassNameBuffer, MAXITEMTXT);
	wsprintfA(szMsg, "windows class name: %s Control ID: %d \n", szClassNameBuffer, lgIdCnt);
	OutputDebugStringA(szMsg);

	if (strcmp(szListViewClass, szClassNameBuffer) == 0)
	{
		ghPtLv = hWnd;
		
		wsprintfA(szMsg, "List View window handle: %d \n", (int)ghPtLv);
		OutputDebugStringA(szMsg);
		return FALSE;
	}

	return TRUE;
}

/// <summary>
/// Initialise the Clock Display Window
/// </summary>
/// <returns>Not used</returns>
BOOL InitClockDis(void)
{
	HDC hDC;
	int nHeight = 0;

	hDC = GetDC(ghMain);
	nHeight = -MulDiv(18, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ghFntClk = CreateFont(nHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Courier New"));
	SendMessageA(ghClock, WM_SETFONT, (WPARAM)ghFntClk, (LPARAM)TRUE);
	SendMessageA(ghClock, WM_SETTEXT, (WPARAM)0, (LPARAM)"00:00");

	return TRUE;
}

/// <summary>
/// Start the Mic timer
/// </summary>
void MicTimerStart(void)
{
	SendMessageA(ghClock, WM_SETTEXT, 0, (LPARAM)"00:00");
	giMicTimerSeconds = 0;
	SetTimer(ghMain, IDT_TIMER,1000,0);
}

/// <summary>
/// Reset the Timer
/// </summary>
void MicTimerReset(void)
{
	KillTimer(ghMain, IDT_TIMER);
	giMicTimerSeconds = 0;
	SendMessageA(ghClock, WM_SETTEXT, 0, (LPARAM)"00:00");
}

/// <summary>
/// Measuring the time a nick using the mic 
/// </summary>
void TimerMicTick(void)
{
	char szClock[MAXITEMTXT] = { '\0' };
	char szMsg[MAXITEMTXT] = { '\0' };
	int iX = 60;
	int iMin;
	int iSec;

	giMicTimerSeconds++;

	iMin = giMicTimerSeconds / iX;
	iSec = giMicTimerSeconds % iX;

	sprintf_s(szClock, MAXITEMTXT, "%02d:%02d", iMin, iSec);
	SendMessageA(ghClock, WM_SETTEXT, 0, (LPARAM)szClock);

	// Work out when to send text to Paltalk
	if (giMicTimerSeconds % giInterval == 0)
	{
		sprintf_s(szMsg, MAXITEMTXT, "%s on Mic for: %d:%02d min.", gszCurrentNick,iMin, iSec);
		CopyPaste2Paltalk(szMsg);
	}
}

/// <summary>
/// This is where the Mic timing logic happens 
/// </summary>
void MonitorTimerTick(void)
{
	// Failed to get current nick
	if (!GetMicUser()) return;
	if (strlen(gszCurrentNick) < 2 && strlen(gszSavedNick) < 2) return;
	// no change keep going
	if (strcmp(gszCurrentNick, gszSavedNick) == 0)
	{
		iDrp = 0; // Reset dropout counter
	}
	// No nick but there was one before
	else if (strlen(gszCurrentNick) < 2 && strlen(gszSavedNick) > 2)
	{
		iDrp++; //to tolerate mic dropout
		char szTemp[MAXITEMTXT] = { '\0' };
		sprintf_s(szTemp, "Mic dropout count %d", iDrp);
		OutputDebugStringA(szTemp);
		if (iDrp > 4) // 5 second dropout
		{
			MicTimerReset(); //Stop the mic timimg
			sprintf_s(gszSavedNick, "a"); // Reseting the saved nick
			sprintf_s(szTemp, "5 dropouts: %d Reset Mic timer",iDrp);
			OutputDebugStringA(szTemp);
			iDrp = 0;
		}
	}
	// New nick on mic -> Start Mic Timer
	else if (strcmp(gszCurrentNick, gszSavedNick) != 0)
	{
		SYSTEMTIME sytUtc;
		char szMsg[MAXITEMTXT] = { '\O' };

		MicTimerStart();

		GetSystemTime(&sytUtc);

		sprintf_s(szMsg, "Start: %s at: %02d:%02d:%02d UTC", gszCurrentNick, sytUtc.wHour,sytUtc.wMinute,sytUtc.wSecond);
		CopyPaste2Paltalk(szMsg);

		strcpy_s(gszSavedNick, gszCurrentNick);
	}
}

	


/// <summary>
/// Gets the current Nick on Mic
/// </summary>
/// <returns>When this TRUE the current global </returns>
BOOL GetMicUser(void)
{
	if (!ghPtLv) return FALSE;

	char szItemName[MAXITEMTXT] = { 'A','B','C','D' };  //wchar_t
	char szItemNameRead[MAXITEMTXT] = { '0' };  //wchar_t
	char szOnMicName[MAXITEMTXT] = { '0' };
	char szOut[MAXITEMTXT] = { '0' };
	char szMsg[MAXITEMTXT] = { '0' };

	BOOL bReturn = FALSE;

	const int iSizeOfItemNameBuff = MAXITEMTXT * sizeof(char); //wchar_t
	LPSTR pXItemNameBuff = NULL;

	LVITEMA lviNick = { 0 };
	DWORD dwProcId;
	VOID* pM;
	HANDLE hProc;
	int iI, iNicks;
	int iImg = 0;

	LVITEMA lviRead = { 0 };

	GetWindowThreadProcessId(ghPtLv, &dwProcId);
	hProc = OpenProcess(
		PROCESS_VM_OPERATION |
		PROCESS_VM_READ |
		PROCESS_VM_WRITE, FALSE, dwProcId);

	if (hProc == NULL)return FALSE;

	pM = VirtualAllocEx(hProc, NULL, sizeof(LVITEMA),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pM == NULL) return FALSE;

	pXItemNameBuff = (LPSTR)VirtualAllocEx(hProc, NULL, iSizeOfItemNameBuff,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pXItemNameBuff == NULL) return FALSE;

	iNicks = ListView_GetItemCount(ghPtLv);

	for (iI = 0; iI < iNicks; iI++)
	{

		lviNick.mask = LVIF_IMAGE | LVIF_TEXT;
		lviNick.pszText = pXItemNameBuff;
		lviNick.cchTextMax = MAXITEMTXT - 1;
		lviNick.iItem = iI;
		lviNick.iSubItem = 0;

		if (!WriteProcessMemory(hProc, pM, &lviNick, sizeof(lviNick), NULL))
		{
			bReturn = FALSE;
			break;
		}
		
		if (!SendMessage(ghPtLv, 4171, (WPARAM)iI, (LPARAM)pM))
		{
			bReturn = FALSE;
			break;
		}
	
		if (!ReadProcessMemory(hProc, pM, &lviRead, sizeof(lviRead), NULL))
		{
			bReturn = FALSE;
			break;
		}
	
		iImg = lviRead.iImage;

		if (iImg == 10)
		{
			if (!SendMessage(ghPtLv, 4141, (WPARAM)iI, (LPARAM)pM))
			{
				bReturn = FALSE;
				break;
			}
		
			if (!ReadProcessMemory(hProc, lviRead.pszText, &szItemNameRead, sizeof(szItemNameRead), NULL))
			{
				bReturn = FALSE;
				break;
			}

			sprintf_s(szMsg, "Image: %d Nickname: %s \n", iImg, szItemNameRead);
			OutputDebugStringA(szMsg);

			sprintf_s(gszCurrentNick, "%s", szItemNameRead);
			bReturn = TRUE;
		}
				
	}
	
	// Cleanup 
	if (pM != NULL) VirtualFreeEx(hProc, pM, 0, MEM_RELEASE);
	if (pXItemNameBuff != NULL) VirtualFreeEx(hProc, pXItemNameBuff, 0, MEM_RELEASE);

	CloseHandle(hProc);

	return bReturn;
}

/// <summary>
/// Setting up the intervals 
/// </summary>
/// <returns>Not used</returns>
BOOL InitIntervals(void)
{
	wchar_t wsTemp[256] = { '\0' };
	BOOL bErr = FALSE;
	int iIndex = 0;
	int iSec = 0;
	int iMin = 0;

	for (int i = 30; i <= 180; i=i+30 )
	{
		iMin = i / 60;
		iSec = i % 60;
		swprintf_s(wsTemp, L"%d:%02d", iMin, iSec);
		iIndex = SendMessageW(ghInterval, CB_ADDSTRING, (WPARAM)0, (LPARAM)wsTemp);
		SendMessageW(ghInterval, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)i);
	}

	return TRUE;
}

/// <summary>
/// Sending text to Paltalk room
/// </summary>
/// <param name="szMsg">Text to send</param>
void CopyPaste2Paltalk(char* szMsg)
{
	char szOut[MAXITEMTXT] = { '\0' };
	OutputDebugStringA(gszCurrentNick);

	if (strlen(gszCurrentNick) < 2) return;
	else if (SendMessageA(ghCbSend, BM_GETCHECK, (WPARAM)0, (LPARAM)0) != BST_CHECKED) return;
	
	BOOL bRes = SetForegroundWindow(ghPtMain);
	Sleep(500);

	sprintf_s(szOut, MAXITEMTXT, "*** %s ***", szMsg);

	for (int i = 0; i < strlen(szOut); i++)
	{
		SendMessageA(ghPtMain, WM_CHAR, (WPARAM)szOut[i], 0);
	}
	SendMessageA(ghPtMain, WM_KEYDOWN, (WPARAM)VK_RETURN, 0);
	SendMessageA(ghPtMain, WM_KEYUP, (WPARAM)VK_RETURN, 0);
		
}

/// Start Monitoring the mic que
void StartStopMonitoring(void)
{
	if (!ghPtRoom)
	{
		msga("Error: No Paltalk Room!\n [Get Pt] first!");
		return;
	}
	if (!gbMonitor)
	{
		SetTimer(ghMain, IDT_TIMERU, 1000, NULL);
		gbMonitor = TRUE;
		SendDlgItemMessageW(ghMain, IDC_BUTTON_START, WM_SETTEXT, 0, (LPARAM)L"Stop");
	}
	else
	{
		KillTimer(ghMain, IDT_TIMERU);
		sprintf_s(gszSavedNick, "a");
		MicTimerReset();
		gbMonitor = FALSE;
		SendDlgItemMessageW(ghMain, IDC_BUTTON_START, WM_SETTEXT, 0, (LPARAM)L"Start");
	}
}