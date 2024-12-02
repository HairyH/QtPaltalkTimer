
/*********************************/
/* (c) HairyH 2020               */
/*********************************/
#include "main.h"
// Global Variables
HINSTANCE	hInst = 0;
HWND		ghMain = 0;
HWND        ghPtMain = 0;
HWND		ghEdit = 0;
HWND		ghInterval = 0;
HWND		ghClock = 0;

// Paltalk Handles
HWND ghPtRoom = NULL, ghPtLv = NULL;

// Font handles
HFONT ghFntClk = NULL;

// Timer related globals
SYSTEMTIME gsysTime = { 0 };
BOOL gbMonitor = FALSE;
BOOL gbRun = FALSE;
//BOOL gbPtHandles[6];
BOOL gbNStart = FALSE;

int  giSecLimit = 0, giSecTime = 0;
int giInterval = 30;
// Nick related globals
char gszSavedNick[MAX_PATH] = { '0' };
char gszCurrentNick[MAX_PATH] = { '0' };
int giDropOut = 4;
int iMaxNicks = 0;

//Quick Messagebox macro
#define msga(x) msgba(ghMain,x)
// Quick Messagebox WIDE String
#define msgw(x) msgbw(ghMain,x)

// Function prototypes
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void GetPaltalkWindows(void);
BOOL CALLBACK EnumPaltalkWindows(HWND hWnd, LPARAM lParam);
BOOL InitClockDis(void);
BOOL InitIntervals(void);
void OnTimerStart(void);
void OnTimerReset(void);
void OnTimerTick(UINT id);
BOOL GetMicUser(void);
BOOL SendTextToPaltalk(wchar_t* wsMsg);
void StartStopMonitoring(void);

/// Main entry point of the app 
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	hInst = hInstance;
	InitCommonControls();
	LoadLibrary(L"riched20.dll"); // comment if richedit is not used
	// TODO: Add any initiations as needed
	return DialogBox(hInst, MAKEINTRESOURCE(IDD_DLGMAIN), NULL, (DLGPROC)DlgMain);
}

//
/// Callback main message loop for the Application
//
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ghMain = hwndDlg;
		ghInterval = GetDlgItem(hwndDlg, IDC_COMBO_INTERVAL);
		ghClock = GetDlgItem(hwndDlg, IDC_EDIT_CLOCK);
		InitClockDis();
		InitIntervals();
	}
	break; // return TRUE;
	case WM_CLOSE:
	{
		DeleteObject(ghFntClk);
		EndDialog(hwndDlg, 0);
	}
	return TRUE;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDCANCEL:
				{
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
					//StartStopMonitoring();
					GetMicUser();
				}
				return TRUE;
			case IDC_CHECK1:
				{
					gbNStart = IsDlgButtonChecked(ghMain, IDC_CHECK1);
				}
			case IDC_COMBO_INTERVAL:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						int iSel = SendMessage(ghInterval, CB_GETCURSEL, 0, 0);
						giInterval = SendMessage(ghInterval, CB_GETITEMDATA, (WPARAM)iSel, 0);
						char szTemp[100] = { 0 };
						sprintf_s(szTemp, "giInterval = %d \n", giInterval);
						OutputDebugStringA(szTemp);
					}
				}
				return TRUE;
		}
	}
	case WM_TIMER:
	{
		OnTimerTick((UINT)wParam);
	}

	default:
		return FALSE;
	}
	return FALSE;
}

/// Initialise Paltalk Control Handles
void GetPaltalkWindows(void)
{
	char szWinText[200] = { '0' };
	char szTitle[256] = { '0' };

	// Paltalk chat room window
	ghPtRoom = NULL; ghPtLv = NULL;

	// Getting the chat room window handle
	ghPtRoom = FindWindowW(L"DlgGroupChat Window Class", 0); // this is to find nick list
	ghPtMain = FindWindowW(L"Qt5150QWindowIcon", 0);  // thi is to send text 

	if (ghPtRoom) // we got it
	{
		// Get the current thread ID and the target window's thread ID
		DWORD targetThreadId = GetWindowThreadProcessId(ghPtMain, 0);
		DWORD currentThreadId = GetCurrentThreadId();
		// Attach the input of the current thread to the target window's thread
		AttachThreadInput(currentThreadId, targetThreadId, true);
		// Bring the window to the foreground
		bool bRet = SetForegroundWindow(ghPtMain);
		// Give some time for the window to come into focus
		Sleep(500);
		// Detach input once done
		AttachThreadInput(currentThreadId, targetThreadId, false);
		// Make the Paltalk Room window the HWND_TOPMOST 
		SetWindowPos(ghPtMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		GetWindowTextA(ghPtRoom, szWinText, 200);
		// Set the title text to indicate which room we are timing
		sprintf_s(szTitle, 255, "Timing: %s", szWinText);
		SendDlgItemMessageA(ghMain, IDC_STATIC_TITLE, WM_SETTEXT, (WPARAM)0, (LPARAM)szTitle);
		// Finding the chat room window controls handles
		EnumChildWindows(ghPtRoom, EnumPaltalkWindows, 0);

	}
	else
	{
		msga("No Paltalk Window Found!");
	}

}

/// Enumeration Callback to Find the Control Windows
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

		sprintf_s(szMsg, "List View window handle: %p \n", ghPtLv);
		OutputDebugStringA(szMsg);
		return FALSE;
	}

	return TRUE;
}

/// Initialise Intervals Combo
BOOL InitIntervals(void)
{
	wchar_t wsTemp[256] = { '\0' };
	BOOL bErr = FALSE;
	int iIndex = 0;
	int iSec = 0;
	int iMin = 0;

	for (int i = 30; i <= 180; i = i + 30)
	{
		iMin = i / 60;
		iSec = i % 60;
		swprintf_s(wsTemp, L"%d:%02d", iMin, iSec);
		iIndex = SendMessageW(ghInterval, CB_ADDSTRING, (WPARAM)0, (LPARAM)wsTemp);
		SendMessageW(ghInterval, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)i);
	}

	SendMessageW(ghInterval, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	giInterval = SendMessageW(ghInterval, CB_GETITEMDATA, (WPARAM)0, (LPARAM)0);
	return TRUE;
}




/// Initialise the Clock Display Window
BOOL InitClockDis(void)
{
	HDC hDC;
	int nHeight = 0;

	hDC = GetDC(ghMain);
	nHeight = -MulDiv(20, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ghFntClk = CreateFont(nHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Courier New"));
	SendMessageA(ghClock, WM_SETFONT, (WPARAM)ghFntClk, (LPARAM)TRUE);
	SendMessageA(ghClock, WM_SETTEXT, (WPARAM)0, (LPARAM)"00:00");

	return TRUE;
}

/// Start the Timer
void OnTimerStart(void)
{
	

}
/// Reset the Timer
void OnTimerReset(void)
{
	
}

void OnTimerTick(UINT id)
{

}

/// Get the Mic user
BOOL GetMicUser(void)
{
	if (!ghPtLv) return FALSE;
		
	char szOut[MAX_PATH] = { '\0' };
	char szMsg[MAX_PATH] = { '\0' };
	wchar_t wsMsg[MAX_PATH] = { '\0' };
	BOOL bRet = FALSE;

	const int iSizeOfItemNameBuff = MAX_PATH * sizeof(char); //wchar_t
	LPSTR pXItemNameBuff = NULL;

	LVITEMA lviNick = { 0 };
	DWORD dwProcId;
	VOID* vpMemLvi;
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

	vpMemLvi = VirtualAllocEx(hProc, NULL, sizeof(LVITEMA),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (vpMemLvi == NULL) return FALSE;

	pXItemNameBuff = (LPSTR)VirtualAllocEx(hProc, NULL, iSizeOfItemNameBuff,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pXItemNameBuff == NULL) return FALSE;

	iNicks = ListView_GetItemCount(ghPtLv);
	if (iMaxNicks < iNicks)
	{
		iMaxNicks = iNicks;
		char szTemp[100] = { 0 };
		sprintf_s(szTemp, "%d", iMaxNicks);
		SendDlgItemMessageA(ghMain, IDC_EDIT2, WM_SETTEXT, (WPARAM)0, (LPARAM)szTemp);
	}
	sprintf_s(szMsg, "Number of nicks: %d \n", iNicks);
	OutputDebugStringA(szMsg);

	for (int i = 0; i < iNicks; i++)
	{

		lviNick.mask = LVIF_IMAGE | LVIF_TEXT;
		lviNick.pszText = pXItemNameBuff;
		lviNick.cchTextMax = MAX_PATH - 1;
		lviNick.iItem = i;
		lviNick.iSubItem = 0;

		WriteProcessMemory(hProc, vpMemLvi, &lviNick, sizeof(lviNick), NULL);
		
		SendMessage(ghPtLv, 4171, (WPARAM)i, (LPARAM)vpMemLvi); // 4171
		
		ReadProcessMemory(hProc, vpMemLvi, &lviRead, sizeof(lviRead), NULL);

		iImg = lviRead.iImage;
		//sprintf_s(szMsg, "Item index: %d iImg: %d \n", i, iImg);
		//OutputDebugStringA(szMsg);

		if(iImg == 10)
		{ 
			SendMessage(ghPtLv, 4141, (WPARAM)i, (LPARAM)vpMemLvi);    // 4141
			
			ReadProcessMemory(hProc, lviRead.pszText, &gszCurrentNick, sizeof(gszCurrentNick), NULL);
			bRet = TRUE;
			break;
		}
		else
		{
			sprintf_s(gszCurrentNick, "a");
			bRet = FALSE;
		}
		
	}

	sprintf_s(szMsg, "Image: %d Nickname: %s \n",  iImg, gszCurrentNick);
	OutputDebugStringA(szMsg);
	// Cleanup 
	if (vpMemLvi != NULL) VirtualFreeEx(hProc, vpMemLvi, 0, MEM_RELEASE);
	if (pXItemNameBuff != NULL) VirtualFreeEx(hProc, pXItemNameBuff, 0, MEM_RELEASE);
CloseHandle(hProc);

	return bRet;
}

/// Sending Text to Paltalk
BOOL SendTextToPaltalk(wchar_t* wsMsg)
{
	//SendMessageW(ghPtOut, EM_REPLACESEL, 0, (LPARAM)wsMsg);
	//SendMessageW(ghPtSendPr, WM_COMMAND, (WPARAM)IDC_PTSEND, (LPARAM)0);
	return TRUE;
}

/// Start Monitoring the mic que
void StartStopMonitoring(void)
{
	
}
