#include "preset_editor.h"
#include "gamma_engine.h"
#include "nvapi_controller.h"
#include "resource.h"
#include <commctrl.h>
#include <string>
#include <cstdio>

namespace {

static HWND g_hwndEditor = NULL;
static HWND g_hwndParent = NULL;  // Hidden window, for posting WM_HOTKEYS_CHANGED
static AppConfig* g_pConfig = nullptr;
static AppConfig g_savedConfig;
static int g_currentPresetIndex = 0;

static HWND g_hwndCombo = NULL;
static HWND g_hwndBrightnessSlider = NULL;
static HWND g_hwndContrastSlider = NULL;
static HWND g_hwndGammaSlider = NULL;
static HWND g_hwndVibranceSlider = NULL;
static HWND g_hwndHueSlider = NULL;
static HWND g_hwndBrightnessValue = NULL;
static HWND g_hwndContrastValue = NULL;
static HWND g_hwndGammaValue = NULL;
static HWND g_hwndVibranceValue = NULL;
static HWND g_hwndHueValue = NULL;
static HWND g_hwndHotkeyDefault = NULL;
static HWND g_hwndHotkeyPreset1 = NULL;
static HWND g_hwndHotkeyPreset2 = NULL;

static const char* EDITOR_CLASS = "ColorSwitcherEditor";

// Convert between Win32 HOTKEY_CLASS format and RegisterHotKey format.
// HOTKEY_CLASS uses HOTKEYF_* flags in the high byte, RegisterHotKey uses MOD_* flags.
static WORD HotkeyBindingToControl(const HotkeyBinding& hk) {
    if (hk.vk == 0) return 0;
    BYTE modFlags = 0;
    if (hk.modifiers & MOD_SHIFT)   modFlags |= HOTKEYF_SHIFT;
    if (hk.modifiers & MOD_CONTROL) modFlags |= HOTKEYF_CONTROL;
    if (hk.modifiers & MOD_ALT)     modFlags |= HOTKEYF_ALT;
    return MAKEWORD(static_cast<BYTE>(hk.vk), modFlags);
}

static HotkeyBinding ControlToHotkeyBinding(WORD val) {
    HotkeyBinding hk;
    hk.vk = LOBYTE(val);
    BYTE modFlags = HIBYTE(val);
    hk.modifiers = 0;
    if (modFlags & HOTKEYF_SHIFT)   hk.modifiers |= MOD_SHIFT;
    if (modFlags & HOTKEYF_CONTROL) hk.modifiers |= MOD_CONTROL;
    if (modFlags & HOTKEYF_ALT)     hk.modifiers |= MOD_ALT;
    return hk;
}

static void ReadHotkeysFromControls() {
    g_pConfig->hotkeyDefault = ControlToHotkeyBinding(
        static_cast<WORD>(SendMessage(g_hwndHotkeyDefault, HKM_GETHOTKEY, 0, 0)));
    g_pConfig->hotkeyPreset1 = ControlToHotkeyBinding(
        static_cast<WORD>(SendMessage(g_hwndHotkeyPreset1, HKM_GETHOTKEY, 0, 0)));
    g_pConfig->hotkeyPreset2 = ControlToHotkeyBinding(
        static_cast<WORD>(SendMessage(g_hwndHotkeyPreset2, HKM_GETHOTKEY, 0, 0)));
}

static void LoadHotkeysToControls() {
    SendMessage(g_hwndHotkeyDefault, HKM_SETHOTKEY,
        HotkeyBindingToControl(g_pConfig->hotkeyDefault), 0);
    SendMessage(g_hwndHotkeyPreset1, HKM_SETHOTKEY,
        HotkeyBindingToControl(g_pConfig->hotkeyPreset1), 0);
    SendMessage(g_hwndHotkeyPreset2, HKM_SETHOTKEY,
        HotkeyBindingToControl(g_pConfig->hotkeyPreset2), 0);
}

ColorPreset& CurrentPreset() {
    return g_currentPresetIndex == 0 ? g_pConfig->preset1 : g_pConfig->preset2;
}

void UpdateValueLabels() {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", CurrentPreset().brightness);
    SetWindowTextA(g_hwndBrightnessValue, buf);
    snprintf(buf, sizeof(buf), "%.2f", CurrentPreset().contrast);
    SetWindowTextA(g_hwndContrastValue, buf);
    snprintf(buf, sizeof(buf), "%.2f", CurrentPreset().gamma);
    SetWindowTextA(g_hwndGammaValue, buf);
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(CurrentPreset().vibrance));
    SetWindowTextA(g_hwndVibranceValue, buf);
    snprintf(buf, sizeof(buf), "%d\xC2\xB0", static_cast<int>(CurrentPreset().hue));
    SetWindowTextA(g_hwndHueValue, buf);
}

void LoadSlidersFromPreset() {
    const ColorPreset& p = CurrentPreset();
    SendMessage(g_hwndBrightnessSlider, TBM_SETPOS, TRUE, static_cast<LPARAM>(p.brightness * 100));
    SendMessage(g_hwndContrastSlider, TBM_SETPOS, TRUE, static_cast<LPARAM>(p.contrast * 100));
    SendMessage(g_hwndGammaSlider, TBM_SETPOS, TRUE, static_cast<LPARAM>(p.gamma * 100));
    SendMessage(g_hwndVibranceSlider, TBM_SETPOS, TRUE, static_cast<LPARAM>(p.vibrance));
    SendMessage(g_hwndHueSlider, TBM_SETPOS, TRUE, static_cast<LPARAM>(p.hue));
    UpdateValueLabels();
}

void ReadSlidersToPreset() {
    ColorPreset& p = CurrentPreset();
    p.brightness = SendMessage(g_hwndBrightnessSlider, TBM_GETPOS, 0, 0) / 100.0f;
    p.contrast = SendMessage(g_hwndContrastSlider, TBM_GETPOS, 0, 0) / 100.0f;
    p.gamma = SendMessage(g_hwndGammaSlider, TBM_GETPOS, 0, 0) / 100.0f;
    p.vibrance = static_cast<float>(SendMessage(g_hwndVibranceSlider, TBM_GETPOS, 0, 0));
    p.hue = static_cast<float>(SendMessage(g_hwndHueSlider, TBM_GETPOS, 0, 0));
}

void LivePreview() {
    ReadSlidersToPreset();
    UpdateValueLabels();
    GammaEngine::ApplyPreset(CurrentPreset());
}

HWND CreateLabel(HWND parent, HINSTANCE hInst, const char* text, int x, int y, int w, int h) {
    return CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, NULL, hInst, NULL);
}

HWND CreateTrackbar(HWND parent, HINSTANCE hInst, int id, int x, int y, int w, int h,
                    int rangeMin, int rangeMax, bool enabled = true) {
    DWORD style = WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS;
    if (!enabled) style |= WS_DISABLED;
    HWND hwnd = CreateWindowExA(0, TRACKBAR_CLASSA, "",
        style,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hInst, NULL);
    SendMessage(hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(rangeMin, rangeMax));
    SendMessage(hwnd, TBM_SETTICFREQ, (rangeMax - rangeMin) / 10, 0);
    return hwnd;
}

HWND CreateHotkeyControl(HWND parent, HINSTANCE hInst, int id, int x, int y, int w, int h) {
    return CreateWindowExA(0, HOTKEY_CLASSA, "",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hInst, NULL);
}

LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance;
        bool nvapiOk = NvapiController::IsAvailable();

        int labelW = 70, sliderX = 80, sliderW = 180, valueX = 270, valueW = 50;
        int y = 10, rowH = 35;

        // Preset selector
        CreateLabel(hwnd, hInst, "Preset:", 10, y + 2, 50, 20);
        g_hwndCombo = CreateWindowExA(0, "COMBOBOX", "",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            65, y, 120, 200, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PRESET_COMBO)),
            hInst, NULL);
        SendMessageA(g_hwndCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Preset 1"));
        SendMessageA(g_hwndCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Preset 2"));
        SendMessage(g_hwndCombo, CB_SETCURSEL, g_currentPresetIndex, 0);

        y += rowH + 5;

        // Brightness
        CreateLabel(hwnd, hInst, "Brightness:", 10, y + 5, labelW, 20);
        g_hwndBrightnessSlider = CreateTrackbar(hwnd, hInst, IDC_BRIGHTNESS_SLIDER,
            sliderX, y, sliderW, 30, 0, 100);
        g_hwndBrightnessValue = CreateLabel(hwnd, hInst, "0.50", valueX, y + 5, valueW, 20);
        y += rowH;

        // Contrast
        CreateLabel(hwnd, hInst, "Contrast:", 10, y + 5, labelW, 20);
        g_hwndContrastSlider = CreateTrackbar(hwnd, hInst, IDC_CONTRAST_SLIDER,
            sliderX, y, sliderW, 30, 0, 100);
        g_hwndContrastValue = CreateLabel(hwnd, hInst, "0.50", valueX, y + 5, valueW, 20);
        y += rowH;

        // Gamma
        CreateLabel(hwnd, hInst, "Gamma:", 10, y + 5, labelW, 20);
        g_hwndGammaSlider = CreateTrackbar(hwnd, hInst, IDC_GAMMA_SLIDER,
            sliderX, y, sliderW, 30, 10, 500);
        g_hwndGammaValue = CreateLabel(hwnd, hInst, "1.00", valueX, y + 5, valueW, 20);
        y += rowH;

        // Vibrance
        CreateLabel(hwnd, hInst, "Vibrance:", 10, y + 5, labelW, 20);
        g_hwndVibranceSlider = CreateTrackbar(hwnd, hInst, IDC_VIBRANCE_SLIDER,
            sliderX, y, sliderW, 30, 0, 100, nvapiOk);
        g_hwndVibranceValue = CreateLabel(hwnd, hInst, "50", valueX, y + 5, valueW, 20);
        y += rowH;

        // Hue
        CreateLabel(hwnd, hInst, "Hue:", 10, y + 5, labelW, 20);
        g_hwndHueSlider = CreateTrackbar(hwnd, hInst, IDC_HUE_SLIDER,
            sliderX, y, sliderW, 30, 0, 359, nvapiOk);
        g_hwndHueValue = CreateLabel(hwnd, hInst, "0\xC2\xB0", valueX, y + 5, valueW, 20);
        y += rowH + 8;

        // Separator line
        CreateWindowExA(0, "STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            10, y, 310, 2, hwnd, NULL, hInst, NULL);
        y += 8;

        // Shortcuts section
        CreateLabel(hwnd, hInst, "Shortcuts", 10, y, 60, 16);
        y += 20;

        int hkLabelW = 60, hkX = 70, hkW = 150;

        CreateLabel(hwnd, hInst, "Default:", 10, y + 3, hkLabelW, 16);
        g_hwndHotkeyDefault = CreateHotkeyControl(hwnd, hInst, IDC_HOTKEY_DEFAULT,
            hkX, y, hkW, 22);
        y += 26;

        CreateLabel(hwnd, hInst, "Preset 1:", 10, y + 3, hkLabelW, 16);
        g_hwndHotkeyPreset1 = CreateHotkeyControl(hwnd, hInst, IDC_HOTKEY_PRESET1,
            hkX, y, hkW, 22);
        y += 26;

        CreateLabel(hwnd, hInst, "Preset 2:", 10, y + 3, hkLabelW, 16);
        g_hwndHotkeyPreset2 = CreateHotkeyControl(hwnd, hInst, IDC_HOTKEY_PRESET2,
            hkX, y, hkW, 22);
        y += 34;

        // Buttons
        CreateWindowExA(0, "BUTTON", "Save",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            140, y, 80, 28, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SAVE_BTN)),
            hInst, NULL);
        CreateWindowExA(0, "BUTTON", "Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            230, y, 80, 28, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CANCEL_BTN)),
            hInst, NULL);

        LoadSlidersFromPreset();
        LoadHotkeysToControls();

        // Set font for all child controls
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
            SendMessage(child, WM_SETFONT, lp, TRUE);
            return TRUE;
        }, reinterpret_cast<LPARAM>(hFont));

        return 0;
    }

    case WM_HSCROLL: {
        HWND slider = reinterpret_cast<HWND>(lParam);
        if (slider == g_hwndBrightnessSlider ||
            slider == g_hwndContrastSlider ||
            slider == g_hwndGammaSlider ||
            slider == g_hwndVibranceSlider ||
            slider == g_hwndHueSlider) {
            LivePreview();
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PRESET_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                ReadSlidersToPreset();
                g_currentPresetIndex = static_cast<int>(SendMessage(g_hwndCombo, CB_GETCURSEL, 0, 0));
                LoadSlidersFromPreset();
                LivePreview();
            }
            break;

        case IDC_SAVE_BTN:
            ReadSlidersToPreset();
            ReadHotkeysFromControls();
            ConfigManager::Save(*g_pConfig);
            if ((g_currentPresetIndex == 0 && g_pConfig->activePreset == "preset1") ||
                (g_currentPresetIndex == 1 && g_pConfig->activePreset == "preset2")) {
                GammaEngine::ApplyPreset(CurrentPreset());
            } else {
                const ColorPreset* active = ConfigManager::GetActivePreset(*g_pConfig);
                if (active) GammaEngine::ApplyPreset(*active);
                else GammaEngine::ApplyDefault();
            }
            // Notify main window to re-register hotkeys
            if (g_hwndParent) PostMessage(g_hwndParent, WM_HOTKEYS_CHANGED, 0, 0);
            DestroyWindow(hwnd);
            break;

        case IDC_CANCEL_BTN: {
            *g_pConfig = g_savedConfig;
            const ColorPreset* active = ConfigManager::GetActivePreset(*g_pConfig);
            if (active) GammaEngine::ApplyPreset(*active);
            else GammaEngine::ApplyDefault();
            DestroyWindow(hwnd);
            break;
        }
        }
        return 0;

    case WM_CLOSE:
        *g_pConfig = g_savedConfig;
        {
            const ColorPreset* active = ConfigManager::GetActivePreset(*g_pConfig);
            if (active) GammaEngine::ApplyPreset(*active);
            else GammaEngine::ApplyDefault();
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        g_hwndEditor = NULL;
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

} // anonymous namespace

namespace PresetEditor {

void Show(HINSTANCE hInstance, HWND hwndParent, AppConfig& config) {
    if (g_hwndEditor) {
        SetForegroundWindow(g_hwndEditor);
        return;
    }

    g_pConfig = &config;
    g_savedConfig = config;
    g_hwndParent = hwndParent;

    if (config.activePreset == "preset2") g_currentPresetIndex = 1;
    else g_currentPresetIndex = 0;

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = EditorWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = EDITOR_CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&wc);

    int winW = 340, winH = 430;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;

    g_hwndEditor = CreateWindowExA(
        WS_EX_TOOLWINDOW,
        EDITOR_CLASS,
        "Customize Presets",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, winW, winH,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(g_hwndEditor, SW_SHOW);
    UpdateWindow(g_hwndEditor);
}

} // namespace PresetEditor
