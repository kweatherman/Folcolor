
// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#include "StdAfx.h"
#include "resource.h"
#include "FolderColorize.h"

extern WCHAR myPathGlobal[MAX_PATH];
extern int iconOffsetGlobal;


// Restore default folder icon by removing the folder "desktop.ini"
static void RestoreFolderIcon(LPWSTR widePath)
{
	// Skip if the folder system flag is not set
	if (widePath && PathIsSystemFolderW(widePath, 0))
	{
		WCHAR initPath[MAX_PATH];
		_snwprintf_s(initPath, MAX_PATH, (MAX_PATH-1), L"%s\\desktop.ini", widePath);

		// ini file exists?
		DWORD attr = GetFileAttributesW(initPath);
		if ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Yes, detect if the desktop.ini setup has other settings like a custom background, tip, etc.
			// Be nice to the user and not delete there custom "desktop.ini" setup for the folder if they have one.
			BOOL keepIt = FALSE;

			// Read in the the whole text block
			FILE *fp = NULL;
			errno_t err = _wfopen_s(&fp, initPath, L"rb");
			if (err == 0)
			{				
				long size = fsize(fp);
				if (size > 0)
				{
					LPSTR buffer = (LPSTR) malloc(size + 1);
					if (buffer)
					{
						// Ensure it's zero terminated
						buffer[size] = 0;

						if (fread_s(buffer, size, size, 1, fp) == 1)
						{							
							// Quick test if the file has a GUID def typical of extended attributes
							if (strchr(buffer, '{'))
								keepIt = TRUE;
							else
							{
								// Expanded search
								_strlwr_s(buffer, (size + 1));
								static const char *strLst[] =
								{
									"[extshellfolderviews]",
									"[viewstate]",
									"iconarea_image=",
									"iconarea_text=",
									"infotip=",
									"nosharing=",
									"logo="
								};

								for (UINT i = 0; i < _countof(strLst); i++)
								{
									if (strstr(buffer, strLst[i]) != NULL)
									{
										keepIt = TRUE;
										break;
									}
								}
							}
						}

						free(buffer);
					}
				}

				fclose(fp);
			}

			if (keepIt)
			{
				// Only remove the icon fields to fall back to the OS default icon
				WritePrivateProfileStringW(L".ShellClassInfo", L"IconFile", NULL, initPath);
				WritePrivateProfileStringW(L".ShellClassInfo", L"IconIndex", NULL, initPath);
				WritePrivateProfileStringW(L".ShellClassInfo", L"IconResource", NULL, initPath);

				// If there no fields left now in the ".ShellClassInfo" section remove it from the desktop.ini
				WCHAR buffer[1024];
				DWORD ppsr = GetPrivateProfileSectionW(L".ShellClassInfo", buffer, _countof(buffer), initPath);
				if (ppsr == 0)
					WritePrivateProfileStringW(L".ShellClassInfo", NULL, NULL, initPath);
			}
			else
			{
				// Detected nothing no significant settings, so we can delete it
				DeleteFileW(initPath);

				// Remove folder "system" flag too
				// #TODO: Looks like other tools don't do this step, any bad use cases?
				PathUnmakeSystemFolderW(widePath);
			}
		}
		else
		{
			// No, remove folder "system" flag
			// #TODO: Looks like other tools don't do this step, any bad use cases?
			PathUnmakeSystemFolderW(widePath);
		}
	}
}


// Set folder color icon for a given folder
void SetFolderColor(int index, LPWSTR folderPath)
{
	//trace("SetFolderColor: %d, \"%S\"\n", index, folderPath);
	if (!folderPath || (index < 0) || (index > COLOR_ICON_COUNT))
		return;

	// Shouldn't happen, but verify it's an exiting folder first
	DWORD attr = GetFileAttributesW(folderPath);
	if ((attr == INVALID_FILE_ATTRIBUTES) || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		return;
	
	// Path to a "desktop.ini"
	WCHAR initPath[MAX_PATH];
	if (_snwprintf_s(initPath, MAX_PATH, (MAX_PATH-1), L"%s\\desktop.ini", folderPath) < 1)
		CRITICAL("Path size limit error!");

	// Folder already has system flag?
	BOOL hasIniAlready = FALSE;
	if (PathIsSystemFolderW(folderPath, 0))
	{
		// Yes, a "desktop.ini" there?
		DWORD attr = GetFileAttributesW(initPath);
		hasIniAlready = ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY));
		if (hasIniAlready)
		{
			// Yes, has an icon entry?
			// SHGetSetFolderCustomSettings() read combines "IconFile", "IconIndex" and "IconResource" types.
			SHFOLDERCUSTOMSETTINGS pfcs;
			ZeroMemory(&pfcs, sizeof(SHFOLDERCUSTOMSETTINGS));
			pfcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
			pfcs.dwMask = FCSM_ICONFILE;
			WCHAR iconPath[MAX_PATH] = { 0 };
			pfcs.pszIconFile = iconPath;
			pfcs.cchIconFile = MAX_PATH;
			if (SUCCEEDED(SHGetSetFolderCustomSettings(&pfcs, folderPath, FCS_READ)) && iconPath[0])
			{
				// Special folder icon path?
				errno_t en = _wcslwr_s(iconPath);
				if (en != 0)
					CRITICAL_API_ERRNO(_wcslwr_s, en);

				if (wcsncmp(iconPath + SIZESTR(L"C:"), L"\\windows\\", SIZESTR(L"\\windows\\")) == 0)
				{
					// Yes, abort
					MessageBoxA(NULL,
						PROJECT_NAME " detects this as possibly a special folder (I.E. \"Downloads\", \"Documents\", \"Music\", etc.) and is not supported since restoring them is complex.\n\n"
						"If you really want to set the color/icon for this folder, manually edit or just delete the existing \"desktop.ini\" (hidden, system) file first.\n"
						"And then if you want to restore a special system folder icon later, it CAN be done through manual restore steps (Web search on how).\n"
						,
						PROJECT_NAME " abort:", (MB_OK | MB_ICONERROR));
					return;
				}
			}
		}
	}

	// Restore default folder icon?
	if (index == COLOR_ICON_COUNT)
	{
		RestoreFolderIcon(folderPath);
		ResetWindowsIconCache();
		return;
	}

	// If the desktop.ini already exists we can't use SHGetSetFolderCustomSettings() if it has other settting because of a bug in it where it will wipe out
	// other sections if "[.ShellClassInfo]" is not at the top.
	if (hasIniAlready)
	{
		// Need to remove the old first to get quick icon refresh on at least Windows 10
		RestoreFolderIcon(folderPath);

		// If desktop.ini is still here it means there are settings that needed to be saved and will have to take the delayed refresh route,
		// else we'll let it fall through and be recreated for the fast refresh option.
		DWORD attr = GetFileAttributesW(initPath);
		if ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY))
		{
			//trace("** hasIniAlready: Had to save the desktop.ini **\n");

			// Write our "IconResource" entry
			WCHAR iconPath[MAX_PATH];
			_snwprintf_s(iconPath, MAX_PATH, (MAX_PATH-1), L"%sFolcolor.exe,%d", myPathGlobal, (index + iconOffsetGlobal));
			WritePrivateProfileStringW(L".ShellClassInfo", L"IconResource", iconPath, initPath);

			// Flush icon cache so the new icon setting take effect eventually
			PathMakeSystemFolderW(folderPath);
			ResetWindowsIconCache();
			return;
		}
		//else
		//	trace("** hasIniAlready: Making a new desktop.ini **\n");
	}

	// Let SHGetSetFolderCustomSettings() do the work of setting the folder as system, creating the "desktop.ini", etc.
	SHFOLDERCUSTOMSETTINGS pfcs;
	ZeroMemory(&pfcs, sizeof(SHFOLDERCUSTOMSETTINGS));
	pfcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
	pfcs.dwMask = FCSM_ICONFILE;

	WCHAR iconPath[MAX_PATH];
	pfcs.pszIconFile = iconPath;
	pfcs.cchIconFile = MAX_PATH;
	_snwprintf_s(iconPath, MAX_PATH, (MAX_PATH-1), L"%sFolcolor.exe", myPathGlobal);
	pfcs.iIconIndex = (index + iconOffsetGlobal);

	HRESULT hr = SHGetSetFolderCustomSettings(&pfcs, folderPath, FCS_FORCEWRITE);
	if (FAILED(hr))
		CRITICAL_API_FAIL(SHGetSetFolderCustomSettings, HRESULT_CODE(hr));
}


// Reset the Windows icon cache and notify all applications that it changed
// Note: Some applications might not process the broadcast WM_SETTINGCHANGE message and thus will still reference cached/old icons.
void ResetWindowsIconCache()
{
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

	DWORD_PTR smResult;
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, &smResult);
}
