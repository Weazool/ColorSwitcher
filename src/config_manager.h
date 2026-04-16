#pragma once
#include "gamma_engine.h"
#include <string>

struct HotkeyBinding {
    unsigned int modifiers = 0;  // MOD_CTRL, MOD_ALT, MOD_SHIFT, MOD_WIN
    unsigned int vk = 0;         // Virtual key code (0 = unbound)
};

struct AppConfig {
    int version = 1;
    std::string activePreset = "default";  // "default", "preset1", "preset2"
    bool autoStart = false;
    ColorPreset preset1 = { 0.5f, 0.5f, 1.0f };
    ColorPreset preset2 = { 0.5f, 0.5f, 1.0f };
    HotkeyBinding hotkeyDefault;
    HotkeyBinding hotkeyPreset1;
    HotkeyBinding hotkeyPreset2;
    bool autoSwitchByProcess = false;
    std::wstring processPath1;  // empty = unassigned
    std::wstring processPath2;
};

namespace ConfigManager {
    // Get the config file path (%APPDATA%\ColorSwitcher\config.json).
    // Creates the directory if it doesn't exist.
    // Falls back to exe directory if %APPDATA% path fails.
    std::string GetConfigPath();

    // Load config from disk. Returns defaults if file missing or corrupt.
    AppConfig Load();

    // Save config to disk.
    bool Save(const AppConfig& config);

    // Get the preset matching the activePreset name.
    // Returns nullptr for "default".
    const ColorPreset* GetActivePreset(const AppConfig& config);
}
