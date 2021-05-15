
#include "StdAfx.h"
#include <versionhelpers.h>
#include "resource.h"
#include "FolderColorize.h"

#define APP_URL "http://www.folcolor.com/"

static BOOL isInstalled = FALSE;
WCHAR myPathGlobal[MAX_PATH] = { 0 };
int iconOffsetGlobal = WIN10_ICON_OFFSET;
extern void Install();
extern int Uninstall();
extern BOOL HasInstallRegistry();


// If there is another instance of us running, pull it into focus and return TRUE
static BOOL FindDoppelganger()
{
	BOOL found = FALSE;

	UINT myPid = GetCurrentProcessId();
	char myName[_MAX_FNAME];
	if (!GetModuleBaseNameA(GetCurrentProcess(), NULL, myName, _countof(myName)))
		strcpy_s(myName, TARGET_NAME);

	PROCESSENTRY32 nfo;
	nfo.dwSize = sizeof(nfo);
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (handle != INVALID_HANDLE_VALUE)
	{
		if (Process32First(handle, &nfo))
		{
			do
			{
				if ((strcmp(myName, nfo.szExeFile) == 0) && (nfo.th32ProcessID != myPid))
				{
					HWND hwnd = GetHwndForPid(nfo.th32ProcessID);
					if(hwnd)
						ForceWindowFocus(hwnd);
					found = TRUE;
					break;
				}

			} while (Process32Next(handle, &nfo));
		}

		CloseHandle(handle);
	}

	return found;
}


// ----------------------------------------------------------------------------
// Custom hyperlink control
#define VISITED_COLOR RGB(160, 160, 260)
#define URL_BACK_COLOR RGB(76, 76, 76)
#define URL_FONT_HEIGHT 18
#define URL_FONT_WIDTH FW_SEMIBOLD
static HFONT tweakedFont = NULL, undlineFont = NULL;
static BOOL isClicking = FALSE, isVisited = FALSE;
static COLORREF urlColor = RGB(245, 245, 245);
static HBRUSH buttonBrush = NULL;
static HCURSOR mouseOverCursor = NULL;

static void OpenLinkUrl()
{
	SHELLEXECUTEINFOA nfo;
	ZeroMemory(&nfo, sizeof(SHELLEXECUTEINFOA));
	nfo.cbSize = sizeof(SHELLEXECUTEINFOA);
	nfo.lpVerb = "open";
	nfo.lpFile = APP_URL;
	nfo.fMask  = SEE_MASK_ASYNCOK;
	nfo.nShow  = SW_SHOWNORMAL;
	ShellExecuteExA(&nfo);
}

static LRESULT CALLBACK HypLinkSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch(uMsg)
	{
		case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, HypLinkSubclass, uIdSubclass);
		break;

		case WM_NCHITTEST:
		return HTCLIENT;
		break;

		// Switch font when use hovers over us
		case WM_MOUSEMOVE:
		{
			if(GetCapture() != hWnd)
			{
				SendMessage(hWnd, WM_SETFONT, (WPARAM) undlineFont, FALSE);
				//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_HighlightColor);
				InvalidateRect(hWnd, NULL, FALSE);
				SetCapture(hWnd);
			}
			else
			{
				RECT rect;
				GetWindowRect(hWnd, &rect);
				POINT pt = { LOWORD(lParam), HIWORD(lParam)};
				ClientToScreen(hWnd, &pt);

				if(!PtInRect(&rect, pt))
				{
					SendMessage(hWnd, WM_SETFONT, (WPARAM) tweakedFont, FALSE);
					//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor);
					InvalidateRect(hWnd, NULL, FALSE);
					ReleaseCapture();
				}
			}
		}
		break;

		// Finger point cursor when over the control
		case WM_SETCURSOR:
		if(mouseOverCursor)
		{
			SetCursor(mouseOverCursor);
			return 1;
		}
		break;

		case WM_LBUTTONDOWN:
		{
			SetFocus(hWnd);
			SetCapture(hWnd);
			isClicking = TRUE;
		}
		break;

		case WM_LBUTTONUP:
		{
			ReleaseCapture();

			if(isClicking)
			{
				isClicking = FALSE;
				POINT pt;
				pt.x = (short) LOWORD(lParam);
				pt.y = (short) HIWORD(lParam);
				ClientToScreen(hWnd, &pt);
				RECT rc;
				GetWindowRect(hWnd, &rc);

				if(PtInRect(&rc,pt))
				{
					if(!isVisited)
					{
						isVisited = TRUE;
						//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor = pcHyperlink->m_VisitedColor);
						urlColor = VISITED_COLOR;
						InvalidateRect(hWnd, NULL, TRUE);
					}

					OpenLinkUrl();
				}
			}
		}
		break;

		case WM_SETFOCUS:
		{
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;

		case WM_KILLFOCUS:
		{
			SendMessage(hWnd, WM_SETFONT, (WPARAM) tweakedFont, FALSE);
			//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;

		case WM_GETDLGCODE:
		return DLGC_WANTCHARS;
		break;

		// Space key activate?
		case WM_KEYDOWN:
		if(wParam == VK_SPACE)
		{
			if(!isVisited)
			{
				isVisited = TRUE;
				//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor = pcHyperlink->m_VisitedColor);
				urlColor = VISITED_COLOR;
				InvalidateRect(hWnd, NULL, TRUE);
			}

			OpenLinkUrl();
			return 0;
		}
		break;

		case WM_KEYUP:
		if(wParam == VK_SPACE)
			return 0;
		break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// ----------------------------------------------------------------------------


// Our dialog Window message handler
static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// Add info to caption
			SetWindowTextA(hWnd, PROJECT_NAME "  Version: " APP_VERSION ",  Build: " __DATE__);

			// Set dialog icon
			HICON hIcon = LoadIconA((HINSTANCE) GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

			// Setup customized hyperlink control
			HWND hWndCtrl = GetDlgItem(hWnd, IDC_HYPERLINK);
			if (hWndCtrl)
			{
				// Font options
				HFONT fontHandle = (HFONT) SendMessage(hWndCtrl, WM_GETFONT, 0, 0);
				if (!fontHandle)
					fontHandle = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

				if (fontHandle)
				{
					LOGFONT lf;
					if (GetObject(fontHandle, sizeof(LOGFONT), &lf) != 0)
					{
						// Emphasized font
						lf.lfHeight = URL_FONT_HEIGHT;
						lf.lfWeight = URL_FONT_WIDTH;
						tweakedFont = CreateFontIndirect(&lf);
						if (tweakedFont)
							SendMessageA(hWndCtrl, WM_SETFONT, WPARAM(tweakedFont), TRUE);

						// Underlined version
						lf.lfUnderline = TRUE;
						undlineFont = CreateFontIndirect(&lf);
					}
				}

				// Hyperlink finger cursor
				static const BYTE curAND[128] =
				{
					0xF9,0xFF,0xFF,0xFF, 0xF0,0xFF,0xFF,0xFF, 0xF0,0xFF,0xFF,0xFF, 0xF0,0xFF,0xFF,0xFF,
					0xF0,0xFF,0xFF,0xFF, 0xF0,0x3F,0xFF,0xFF, 0xF0,0x07,0xFF,0xFF, 0xF0,0x01,0xFF,0xFF,
					0xF0,0x00,0xFF,0xFF, 0x10,0x00,0x7F,0xFF, 0x00,0x00,0x7F,0xFF, 0x00,0x00,0x7F,0xFF,
					0x80,0x00,0x7F,0xFF, 0xC0,0x00,0x7F,0xFF, 0xC0,0x00,0x7F,0xFF, 0xE0,0x00,0x7F,0xFF,
					0xE0,0x00,0xFF,0xFF, 0xF0,0x00,0xFF,0xFF, 0xF0,0x00,0xFF,0xFF, 0xF8,0x01,0xFF,0xFF,
					0xF8,0x01,0xFF,0xFF, 0xF8,0x01,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
					0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
					0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF
				};
				static const BYTE curXOR[128] =
				{
					0x00,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x06,0x00,0x00,0x00,
					0x06,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x06,0xC0,0x00,0x00, 0x06,0xD8,0x00,0x00,
					0x06,0xDA,0x00,0x00, 0x06,0xDB,0x00,0x00, 0x67,0xFB,0x00,0x00, 0x77,0xFF,0x00,0x00,
					0x37,0xFF,0x00,0x00, 0x17,0xFF,0x00,0x00, 0x1F,0xFF,0x00,0x00, 0x0F,0xFF,0x00,0x00,
					0x0F,0xFE,0x00,0x00, 0x07,0xFE,0x00,0x00, 0x07,0xFE,0x00,0x00, 0x03,0xFC,0x00,0x00,
					0x03,0xFC,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
				};
				mouseOverCursor = CreateCursor(GetModuleHandle(NULL), 5, 0, 32, 32, curAND, curXOR);

				// Subclasses the control
				SetWindowSubclass(hWndCtrl, HypLinkSubclass, 1138, 0);
			}

			// Tweak the button font too
			HFONT fontHandle = (HFONT) SendMessage(GetDlgItem(hWnd, IDC_INSTALL_UNINSTALL), WM_GETFONT, 0, 0);
			if (!fontHandle)
				fontHandle = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
			if (fontHandle)
			{
				LOGFONT lf;
				if (GetObject(fontHandle, sizeof(LOGFONT), &lf) != 0)
				{
					// Emphasized font
					lf.lfHeight = 14;
					lf.lfWeight = FW_MEDIUM;
					HFONT buttonFont = CreateFontIndirect(&lf);
					SendMessageA(GetDlgItem(hWnd, IDC_INSTALL_UNINSTALL), WM_SETFONT, WPARAM(buttonFont), TRUE);
					SendMessageA(GetDlgItem(hWnd, IDC_REFRESH), WM_SETFONT, WPARAM(buttonFont), TRUE);
				}
			}

			if (isInstalled)
				SetDlgItemTextA(hWnd, IDC_INSTALL_UNINSTALL, "Uninstall");

			return (INT_PTR) TRUE;
		}
		break;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				// Install/Uninstall
				case IDC_INSTALL_UNINSTALL:
				{
					if (!isInstalled)
					{
						Install();
						isInstalled = TRUE;
						SetDlgItemTextA(hWnd, IDC_INSTALL_UNINSTALL, "Uninstall");
						MessageBoxA(hWnd, "Installation complete.", "Completion:", (MB_OK | MB_ICONASTERISK));
						EndDialog(hWnd, 0);
					}
					else
					{
						if (MessageBoxA(hWnd, "Uninstall " PROJECT_NAME"?", "Confirmation:", (MB_OKCANCEL | MB_ICONQUESTION)) == IDOK)
						{
							int ur = Uninstall();
							isInstalled = FALSE;
							SetDlgItemTextA(hWnd, IDC_INSTALL_UNINSTALL, "Install");

							if (ur == 0)
							{
								char msg[512];
								sprintf_s(msg, sizeof(msg), PROJECT_NAME " registry uninstalled, but to complete the uninstallation manually delete the\n\"%S\"\nfolder after this dialog closes.", myPathGlobal);
								MessageBoxA(hWnd, msg, "Completion:", (MB_OK | MB_ICONASTERISK));
							}
							else
								MessageBoxA(hWnd, PROJECT_NAME " uninstalled.", "Completion:", (MB_OK | MB_ICONASTERISK));

							EndDialog(hWnd, 0);
						}
					}

					return (INT_PTR) TRUE;
				}
				break;

				// Refresh Windows icon cache DB
				case IDC_REFRESH:
				{
					ResetWindowsIconCache();
					return (INT_PTR) TRUE;
				}
				break;
			};
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			DWORD id = GetDlgCtrlID((HWND)lParam);
			if (id == IDC_HYPERLINK)
			{
				HDC hdcStatic = (HDC) wParam;
				SetTextColor(hdcStatic, urlColor);
				SetBkColor(hdcStatic, URL_BACK_COLOR);
				return (INT_PTR) GetStockObject(NULL_PEN);
			}
		}
		break;

		case WM_CTLCOLORBTN:
		{
			if (!buttonBrush)
				buttonBrush = CreateSolidBrush(RGB(128, 100, 100));
			return (LRESULT) buttonBrush;
		}
		break;

		case WM_CLOSE:		
		EndDialog(hWnd, 0);		
		break;
	}

	return (INT_PTR) FALSE;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Should only be one instance running
	if(FindDoppelganger())
		return EXIT_FAILURE;

	// Build installation folder path
	C_ASSERT(_countof(myPathGlobal) >= MAX_PATH);
	HRESULT hr = SHGetSpecialFolderPathW(0, myPathGlobal, CSIDL_PROGRAM_FILES, FALSE);
	if (FAILED(hr))
		CRITICAL_API_FAIL(SHGetSpecialFolderPathW, HRESULT_CODE(hr));
	if (wcscat_s(myPathGlobal, _countof(myPathGlobal), L"\\" INSTALL_FOLDER L"\\") != 0)
		CRITICAL("Path size limit error!");

	// We're installed?
	// Have registry?
	isInstalled = HasInstallRegistry();
	// Or has install folder?
	DWORD attr = GetFileAttributesW(myPathGlobal);
	isInstalled |= ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));
	
	// Icon resource set index for Win10 or Win 7/8
	if (!IsWindows10OrGreater())
		iconOffsetGlobal = WIN7_ICON_OFFSET;	

	// We're passed icon index argument?
	if (isInstalled)
	{
		if(pCmdLine && (wcslen(pCmdLine) > 0))
		{
			// Get passed color index
			LPWSTR indexPtr = wcsstr(pCmdLine, L"" COMMAND_ICON);
			if (indexPtr)
			{
				// Passed folder path			
				LPWSTR folderPtr = wcsstr(pCmdLine, L"" COMMAND_FOLDER);				
				if(folderPtr)
					SetFolderColor(_wtoi(&indexPtr[SIZESTR(COMMAND_ICON)]), &folderPtr[SIZESTR(COMMAND_FOLDER)]);
			}

			return EXIT_SUCCESS;
		}
	}

	return (int) DialogBoxParamA(hInstance, (LPCSTR) IDD_MAIN, 0, &DlgProc, 0);
}