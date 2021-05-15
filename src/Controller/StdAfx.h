
#pragma once
#define WIN32_LEAN_AND_MEAN
#define WINVER       0x0601 // _WIN32_WINNT_WIN7
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <intrin.h>
#pragma intrinsic(memset, memcpy,  strcat, strcmp, strcpy, strlen)

#include "Utility.h"

#define APP_VERSION "1.1"

#define PROJECT_NAME "Folcolor"
#define INSTALL_FOLDER L"Folcolor"

#define REGISTRY_PATH "Directory\\shell\\Folcolor"

#define COMMAND_ICON "--index="
#define COMMAND_FOLDER "--folder="