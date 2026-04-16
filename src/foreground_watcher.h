#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ForegroundWatcher {
    // Install SetWinEventHook for EVENT_SYSTEM_FOREGROUND.
    // notifyHwnd receives WM_FOREGROUND_CHANGED (wParam/lParam unused) on every
    // real foreground-window change. Idempotent — calling Start twice is safe.
    // Returns false if hook could not be installed.
    bool Start(HWND notifyHwnd);

    // Uninstall the hook. Idempotent.
    void Stop();
}
