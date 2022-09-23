
// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#pragma once

// Size of string sans terminator
#define SIZESTR(x) (_countof(x) - 1)

// #pragma message with location
// Examples:
// #pragma message(__LOC__ "important part to be changed")
// #pragma message(__LOC2__ "error C9901: wish that error would exist")
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning MSG: "
#define __LOC2__ __FILE__ "("__STR1__(__LINE__)") : "

// ------------------------------------------------------------------------------------------------

void trace(const char *format, ...);

void ForceWindowFocus(HWND hWnd);
HWND GetHwndForPid(UINT pid);

BOOL DeleteRegistryPath(__in HKEY hKeyRoot, __in LPCSTR subKey);

long fsize(FILE *fp);

// ------------------------------------------------------------------------------------------------

LPSTR GetErrorString(DWORD error, __out_bcount_z(1024) LPSTR buffer);

void CriticalErrorAbort(int line, __in LPCSTR file, __in LPCSTR reason);

#define CRITICAL(reason) { CriticalErrorAbort(__LINE__, __FILE__, reason); }
#define IF_CRITICAL(expr) { if(!(expr)){ CriticalErrorAbort(__LINE__, __FILE__, #expr); }}
#define CRITICAL_API_FAIL(_api, _error) \
{ \
	char buffer[2048], errorBuffer[1024]; \
	_snprintf_s(buffer, sizeof(buffer), SIZESTR(buffer), "%s() failed, Error: 0x%X @ \"%s\"", #_api, _error, GetErrorString(_error, errorBuffer)); \
	CRITICAL(buffer); \
}
#define CRITICAL_API_ERRNO(_api, _err) \
{ \
	char buffer[2048], errorBuffer[1024];; \
	strerror_s(errorBuffer, sizeof(errorBuffer), _err); \
	_snprintf_s(buffer, sizeof(buffer), SIZESTR(buffer), "%s() failed, Error: 0x%X @ \"%s\"", #_api, _err, errorBuffer); \
	CRITICAL(buffer); \
}
