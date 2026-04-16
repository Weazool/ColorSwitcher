#include "foreground_watcher.h"
#include "resource.h"

namespace {
    HWINEVENTHOOK g_hook = NULL;
    HWND g_notifyHwnd = NULL;

    void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd,
                               LONG idObject, LONG idChild,
                               DWORD, DWORD) {
        if (event != EVENT_SYSTEM_FOREGROUND) return;
        if (idObject != OBJID_WINDOW) return;
        if (idChild != CHILDID_SELF) return;
        if (!hwnd) return;
        if (!g_notifyHwnd) return;
        PostMessageW(g_notifyHwnd, WM_FOREGROUND_CHANGED, 0, 0);
    }
}

namespace ForegroundWatcher {

bool Start(HWND notifyHwnd) {
    if (g_hook) return true;  // already started
    g_notifyHwnd = notifyHwnd;
    g_hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        NULL, WinEventProc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    return g_hook != NULL;
}

void Stop() {
    if (g_hook) {
        UnhookWinEvent(g_hook);
        g_hook = NULL;
    }
    g_notifyHwnd = NULL;
}

} // namespace ForegroundWatcher
