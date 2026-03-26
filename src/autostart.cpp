#include "autostart.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace {

const char* RUN_KEY = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const char* VALUE_NAME = "ColorSwitcher";

std::string GetExePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
}

} // anonymous namespace

namespace AutoStart {

bool IsEnabled() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    DWORD type = 0;
    DWORD size = 0;
    bool exists = (RegQueryValueExA(hKey, VALUE_NAME, NULL, &type, NULL, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
}

bool Enable() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;

    std::string exePath = GetExePath();
    LONG result = RegSetValueExA(hKey, VALUE_NAME, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(exePath.c_str()),
        static_cast<DWORD>(exePath.size() + 1));
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

bool Disable() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;

    LONG result = RegDeleteValueA(hKey, VALUE_NAME);
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

} // namespace AutoStart
