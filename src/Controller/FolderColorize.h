
#pragma once

// Amount of color icons we support
// Windows limits the root context menu to 16 (we are 14 icons plus two featured entries)
#define COLOR_ICON_COUNT 14

// Offsets into our embedded icon resource
#define WIN10_ICON_OFFSET 2	// Windows 10 set
#define WIN7_ICON_OFFSET (2 + COLOR_ICON_COUNT)	// Windows 7 & 8 set


void SetFolderColor(int index, LPWSTR folderPath);
void ResetWindowsIconCache();