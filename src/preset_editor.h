#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "config_manager.h"

namespace PresetEditor {
    // Show the preset editor dialog. Non-modal — returns immediately.
    // If already shown, brings existing window to foreground.
    // config is modified in-place on Save; caller should re-save after.
    void Show(HINSTANCE hInstance, HWND hwndParent, AppConfig& config);
}
