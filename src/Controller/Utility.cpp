
// Misc utility support
#include "StdAfx.h"


// Formated output to OutputDebugStringA() for development
void trace(LPCSTR pszFormat, ...)
{
	va_list vl;
	char szBuffer[2048];
	va_start(vl, pszFormat);
	vsprintf_s(szBuffer, (sizeof(szBuffer) - 1), pszFormat, vl);
	szBuffer[sizeof(szBuffer) - 1] = 0;
	va_end(vl);
	OutputDebugStringA(szBuffer);
}

// ------------------------------------------------------------------------------------------------

// Get an error string for a GetLastError() code
LPSTR GetErrorString(DWORD error, __out_bcount_z(1024) LPSTR buffer)
{
	if (!FormatMessageA((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS),
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buffer, 1024, NULL))
	{
		strncpy_s(buffer, 1024, "Unknown", (1024 - 1));
	}
	else
	{
		// Remove the trailing '\r'
		if (LPSTR lineFeed = strstr(buffer, "\r"))
			*lineFeed = 0;
	}
	return buffer;
}


/*
Critical fail abort handler:
Unlike the common design pattern of error handling that attempts to clean itself up to exit by collapsing
backward (hopefully) on the call stack.

Here we take intentionally take advantage of the fact that Windows OS UM virtualization frees
memory, handles, etc., on exit for us automatically.
Intended for critical/fatal error handling where we must exit anyhow.

Combined with helper macros, this system encourages the covering of more, if not all, bad API result cases.
*/
void CriticalErrorAbort(int line, __in LPCSTR file, __in LPCSTR reason)
{
	if (reason)
	{
		if (!file)
			file = "???";

		char buffer[2048];
		_snprintf_s(buffer, sizeof(buffer), "CRITICAL ERROR: \"%s\", File: \"%s\", line: #%d **\n", reason, file, line);
		MessageBoxA(NULL, buffer, PROJECT_NAME ": CRITICAL ERROR!", (MB_ICONSTOP | MB_OK));
	}
	else
		MessageBoxA(NULL, "Unknown error!", PROJECT_NAME ": CRITICAL ERROR!", (MB_ICONSTOP | MB_OK));

	exit(EXIT_FAILURE);
}

// ------------------------------------------------------------------------------------------------

// Force a window into focus
void ForceWindowFocus(HWND hWnd)
{
	SwitchToThisWindow(hWnd, TRUE);
	BringWindowToTop(hWnd);
	SetForegroundWindow(hWnd);
}

// Get a PID's first related HWND if it has one.
// Note: For the simple case used here, any given process could potentially have more than one associated HWND
HWND GetHwndForPid(UINT pid)
{
	HWND hwndNext = FindWindowEx(NULL, NULL, NULL, NULL);
	while (hwndNext)
	{
		DWORD pid2;
		GetWindowThreadProcessId(hwndNext, &pid2);
		if (pid == pid2)
			return hwndNext;
		else
			hwndNext = FindWindowEx(NULL, hwndNext, NULL, NULL);
	};
	return NULL;
}

// ------------------------------------------------------------------------------------------------

// Get a 32bit file size by handle
long fsize(FILE *fp)
{
	long psave, endpos;
	long result = -1;

	if ((psave = ftell(fp)) != -1L)
	{
		if (fseek(fp, 0, SEEK_END) == 0)
		{
			if ((endpos = ftell(fp)) != -1L)
			{
				fseek(fp, psave, SEEK_SET);
				result = endpos;
			}
		}
	}

	return(result);
}

// ------------------------------------------------------------------------------------------------

// Deletes a registry path and all its sub-keys and values
// Based on: https://docs.microsoft.com/en-us/windows/win32/sysinfo/deleting-a-key-with-subkeys
#define SUBKEY_BUFFER_SIZE 2048
static BOOL RegDelnodeRecurse(__in HKEY rootKey, __out_bcount_z(SUBKEY_BUFFER_SIZE) LPSTR subKey)
{
	// First, see if we can delete the root key without having to recurse
	LSTATUS lStatus = RegDeleteKeyA(rootKey, subKey);
	if (lStatus == ERROR_SUCCESS)
		return TRUE;

	// Open key
	HKEY regKey = NULL;
	lStatus = RegOpenKeyExA(rootKey, subKey, 0, KEY_READ, &regKey);
	if (lStatus != ERROR_SUCCESS)
	{
		if (lStatus == ERROR_FILE_NOT_FOUND)
		{
			//printf("Key not found.\n");
			return TRUE;
		}
		else
		{
			//printf("Error opening key.\n");
			return FALSE;
		}
	}

	// Check for an ending slash and add one if it is missing.
	LPSTR endPtr = (subKey + strlen(subKey));
	if (*(endPtr - 1) != '\\')
	{
		*endPtr++ = '\\';
		*endPtr = 0;
	}

	// Enumerate the keys
	char name[MAX_PATH];
	DWORD size = _countof(name);
	FILETIME writeTime;
	lStatus = RegEnumKeyExA(regKey, 0, name, &size, NULL, NULL, NULL, &writeTime);
	if (lStatus == ERROR_SUCCESS)
	{
		do
		{
			*endPtr = 0;
			errno_t er = strcat_s(subKey, SUBKEY_BUFFER_SIZE, name);
			if (er != 0)
				CRITICAL_API_ERRNO(strcat_s, er);
			if (!RegDelnodeRecurse(rootKey, subKey))
				break;

			size = _countof(name);
			lStatus = RegEnumKeyExA(regKey, 0, name, &size, NULL, NULL, NULL, &writeTime);

		} while (lStatus == ERROR_SUCCESS);
	}

	endPtr--;
	*endPtr = 0;

	RegCloseKey(regKey);

	// Try again to delete the root key
	lStatus = RegDeleteKeyA(rootKey, subKey);
	if (lStatus == ERROR_SUCCESS)
		return TRUE;

	return FALSE;
}
//
BOOL DeleteRegistryPath(__in HKEY hKeyRoot, __in LPCSTR subKey)
{
	char subKeyBuffer[SUBKEY_BUFFER_SIZE];
	errno_t er = strcpy_s(subKeyBuffer, _countof(subKeyBuffer), subKey);
	if (er != 0)
		CRITICAL_API_ERRNO(strcpy_s, er);
	return RegDelnodeRecurse(hKeyRoot, subKeyBuffer);
}
