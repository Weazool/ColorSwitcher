#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>

#include "resource.h"
#include "gamma_engine.h"
#include "config_manager.h"
#include "autostart.h"
#include "preset_editor.h"
#include "nvapi_controller.h"
#include "foreground_watcher.h"

// Version
#define APP_VERSION "1.0.0"

// Globals
static HINSTANCE g_hInstance = NULL;
static HWND g_hwndHidden = NULL;
static NOTIFYICONDATAA g_nid = {};
static AppConfig g_config;
static const char* WINDOW_CLASS = "ColorSwitcherHiddenWnd";
static const char* MUTEX_NAME = "ColorSwitcher_SingleInstance";

// Full path (incl. exe name) of the most recent foreground window evaluated by
// WM_FOREGROUND_CHANGED. Fixed-size wchar_t buffer at a stable address so the
// value can be read externally (debugger / ReadProcessMemory). UTF-16LE,
// null-terminated. Empty string before the first foreground event.
// Intentionally non-static: external linkage keeps the symbol trivially
// findable in the .map / PDB. The runtime address is also written to
// %APPDATA%\ColorSwitcher\debug_address.txt on startup.
wchar_t g_lastForegroundPath[MAX_PATH * 2] = L"";

// Forward declarations
static void ShowTrayIcon();
static void RemoveTrayIcon();
static void UpdateTooltip();
static void ShowContextMenu(HWND hwnd);
static void ApplyActivePreset();
static void SwitchPreset(const std::string& presetName);
static std::wstring GetForegroundExePath();
static bool PathsEqualCI(const std::wstring& a, const std::wstring& b);
static std::string PickPresetForPath(const std::wstring& path);
static void ApplyPresetByName(const std::string& name);
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static bool PathsEqualCI(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    if (a.empty()) return true;
    return CompareStringOrdinal(a.c_str(), static_cast<int>(a.size()),
                                b.c_str(), static_cast<int>(b.size()),
                                TRUE) == CSTR_EQUAL;
}

static std::wstring GetForegroundExePath() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return {};

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return {};
    if (pid == GetCurrentProcessId()) return {};  // ignore our own windows

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return {};

    wchar_t buf[MAX_PATH * 2];
    DWORD size = static_cast<DWORD>(sizeof(buf) / sizeof(buf[0]));
    std::wstring result;
    if (QueryFullProcessImageNameW(hProc, 0, buf, &size) && size > 0) {
        result.assign(buf, size);
    }
    CloseHandle(hProc);
    return result;
}

static std::string PickPresetForPath(const std::wstring& path) {
    if (path.empty()) return "default";
    if (!g_config.processPath1.empty() && PathsEqualCI(path, g_config.processPath1))
        return "preset1";
    if (!g_config.processPath2.empty() && PathsEqualCI(path, g_config.processPath2))
        return "preset2";
    return "default";
}

static void ApplyPresetByName(const std::string& name) {
    if (name == "preset1") {
        GammaEngine::ApplyPreset(g_config.preset1);
    } else if (name == "preset2") {
        GammaEngine::ApplyPreset(g_config.preset2);
    } else {
        GammaEngine::ApplyDefault();
    }
    // Note: intentionally does NOT update g_config.activePreset or save config.
    // activePreset keeps its meaning as "the user's last manual choice".
}

static void UpdateTooltip() {
    std::string tip = "ColorSwitcher v" APP_VERSION " - ";
    if (g_config.activePreset == "preset1") tip += "Preset 1";
    else if (g_config.activePreset == "preset2") tip += "Preset 2";
    else tip += "Default";

    strncpy_s(g_nid.szTip, tip.c_str(), sizeof(g_nid.szTip) - 1);
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

static void ShowTrayIcon() {
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwndHidden;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconA(g_hInstance, MAKEINTRESOURCEA(IDI_APP_ICON));

    // Set initial tooltip before adding
    std::string tip = "ColorSwitcher v" APP_VERSION " - ";
    if (g_config.activePreset == "preset1") tip += "Preset 1";
    else if (g_config.activePreset == "preset2") tip += "Preset 2";
    else tip += "Default";
    strncpy_s(g_nid.szTip, tip.c_str(), sizeof(g_nid.szTip) - 1);

    Shell_NotifyIconA(NIM_ADD, &g_nid);
}

static void RemoveTrayIcon() {
    Shell_NotifyIconA(NIM_DELETE, &g_nid);
}

static void ApplyActivePreset() {
    const ColorPreset* preset = ConfigManager::GetActivePreset(g_config);
    if (preset) {
        GammaEngine::ApplyPreset(*preset);
    } else {
        GammaEngine::ApplyDefault();
    }
}

static void SwitchPreset(const std::string& presetName) {
    g_config.activePreset = presetName;
    ApplyActivePreset();
    ConfigManager::Save(g_config);
    UpdateTooltip();
}

static void ShowContextMenu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();

    UINT preset1Flags = MF_STRING | (g_config.activePreset == "preset1" ? MF_CHECKED : MF_UNCHECKED);
    UINT preset2Flags = MF_STRING | (g_config.activePreset == "preset2" ? MF_CHECKED : MF_UNCHECKED);
    UINT defaultFlags = MF_STRING | (g_config.activePreset == "default" ? MF_CHECKED : MF_UNCHECKED);

    AppendMenuA(hMenu, preset1Flags, IDM_PRESET1, "Preset 1");
    AppendMenuA(hMenu, preset2Flags, IDM_PRESET2, "Preset 2");
    AppendMenuA(hMenu, defaultFlags, IDM_DEFAULT, "Default");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    UINT autoSwitchFlags = MF_STRING |
        (g_config.autoSwitchByProcess ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuA(hMenu, autoSwitchFlags, IDM_AUTOSWITCH, "Auto-switch by process");
    AppendMenuA(hMenu, MF_STRING, IDM_CUSTOMIZE, "Customize Presets...");

    UINT autoStartFlags = MF_STRING | (AutoStart::IsEnabled() ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuA(hMenu, autoStartFlags, IDM_AUTOSTART, "Start with Windows");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, IDM_QUIT, "Quit");

    // Required for menu to work properly from tray
    SetForegroundWindow(hwnd);

    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, hwnd, NULL);

    DestroyMenu(hMenu);

    // Required post-TrackPopupMenu to dismiss properly
    PostMessage(hwnd, WM_NULL, 0, 0);
}

static void RegisterHotkey(int id, const HotkeyBinding& hk) {
    if (hk.vk != 0) {
        RegisterHotKey(g_hwndHidden, id, hk.modifiers | MOD_NOREPEAT, hk.vk);
    }
}

static void UnregisterAllHotkeys() {
    UnregisterHotKey(g_hwndHidden, HOTKEY_ID_DEFAULT);
    UnregisterHotKey(g_hwndHidden, HOTKEY_ID_PRESET1);
    UnregisterHotKey(g_hwndHidden, HOTKEY_ID_PRESET2);
}

static void RegisterAllHotkeys() {
    UnregisterAllHotkeys();
    RegisterHotkey(HOTKEY_ID_DEFAULT, g_config.hotkeyDefault);
    RegisterHotkey(HOTKEY_ID_PRESET1, g_config.hotkeyPreset1);
    RegisterHotkey(HOTKEY_ID_PRESET2, g_config.hotkeyPreset2);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_PRESET1:
            SwitchPreset("preset1");
            break;
        case IDM_PRESET2:
            SwitchPreset("preset2");
            break;
        case IDM_DEFAULT:
            SwitchPreset("default");
            break;
        case IDM_CUSTOMIZE:
            PresetEditor::Show(g_hInstance, g_hwndHidden, g_config);
            break;
        case IDM_AUTOSTART: {
            bool wantEnabled = !AutoStart::IsEnabled();
            if (wantEnabled) AutoStart::Enable();
            else AutoStart::Disable();

            bool actualState = AutoStart::IsEnabled();
            if (actualState != wantEnabled) {
                MessageBoxA(hwnd,
                    "Could not change the auto-start setting.\n"
                    "This may be restricted by system policy.",
                    "ColorSwitcher", MB_OK | MB_ICONWARNING);
            }
            g_config.autoStart = actualState;
            ConfigManager::Save(g_config);
            break;
        }
        case IDM_AUTOSWITCH:
            g_config.autoSwitchByProcess = !g_config.autoSwitchByProcess;
            ConfigManager::Save(g_config);
            break;
        case IDM_QUIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;

    case WM_HOTKEYS_CHANGED:
        RegisterAllHotkeys();
        return 0;

    case WM_HOTKEY:
        switch (wParam) {
        case HOTKEY_ID_DEFAULT:
            SwitchPreset("default");
            break;
        case HOTKEY_ID_PRESET1:
            SwitchPreset("preset1");
            break;
        case HOTKEY_ID_PRESET2:
            SwitchPreset("preset2");
            break;
        }
        return 0;

    case WM_FOREGROUND_CHANGED: {
        std::wstring path = GetForegroundExePath();
        lstrcpynW(g_lastForegroundPath, path.c_str(),
                  sizeof(g_lastForegroundPath) / sizeof(wchar_t));
        if (g_config.autoSwitchByProcess) {
            ApplyPresetByName(PickPresetForPath(path));
        }
        return 0;
    }

    case WM_DISPLAYCHANGE:
        ApplyActivePreset();
        return 0;

    case WM_POWERBROADCAST:
        if (wParam == PBT_APMRESUMEAUTOMATIC) {
            ApplyActivePreset();
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_DESTROY:
        ForegroundWatcher::Stop();
        UnregisterAllHotkeys();
        GammaEngine::ApplyDefault();
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Single instance check
    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0;
    }

    g_hInstance = hInstance;

    // Initialize common controls (for trackbars in editor)
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);

    // Initialize NVAPI (vibrance/hue support — silently skipped if no NVIDIA GPU)
    NvapiController::Init();

    // Load config and apply
    g_config = ConfigManager::Load();

    // Register hidden window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClassExA(&wc);

    // Use NULL parent (not HWND_MESSAGE) so we receive WM_DISPLAYCHANGE
    // and WM_POWERBROADCAST broadcast messages.
    g_hwndHidden = CreateWindowExA(0, WINDOW_CLASS, "ColorSwitcher",
        0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    if (!g_hwndHidden) {
        CloseHandle(hMutex);
        return 1;
    }

    // Apply active preset
    ApplyActivePreset();

    // Show tray icon
    ShowTrayIcon();

    // Register global hotkeys
    RegisterAllHotkeys();
    ForegroundWatcher::Start(g_hwndHidden);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    NvapiController::Shutdown();
    CloseHandle(hMutex);
    return static_cast<int>(msg.wParam);
}
