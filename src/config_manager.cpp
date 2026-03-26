#include "config_manager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <shlobj.h>
#include <algorithm>

using json = nlohmann::json;

namespace {

float ClampBrightness(float v) { return std::clamp(v, 0.0f, 1.0f); }
float ClampContrast(float v) { return std::clamp(v, 0.0f, 1.0f); }
float ClampGamma(float v) { return std::clamp(v, 0.1f, 5.0f); }
float ClampVibrance(float v) { return std::clamp(v, 0.0f, 100.0f); }
float ClampHue(float v) { return std::clamp(v, 0.0f, 359.0f); }

ColorPreset ParsePreset(const json& j, const ColorPreset& fallback) {
    ColorPreset p = fallback;
    if (j.contains("brightness") && j["brightness"].is_number())
        p.brightness = ClampBrightness(j["brightness"].get<float>());
    if (j.contains("contrast") && j["contrast"].is_number())
        p.contrast = ClampContrast(j["contrast"].get<float>());
    if (j.contains("gamma") && j["gamma"].is_number())
        p.gamma = ClampGamma(j["gamma"].get<float>());
    if (j.contains("vibrance") && j["vibrance"].is_number())
        p.vibrance = ClampVibrance(j["vibrance"].get<float>());
    if (j.contains("hue") && j["hue"].is_number())
        p.hue = ClampHue(j["hue"].get<float>());
    return p;
}

HotkeyBinding ParseHotkey(const json& j) {
    HotkeyBinding hk;
    if (j.contains("modifiers") && j["modifiers"].is_number())
        hk.modifiers = j["modifiers"].get<unsigned int>();
    if (j.contains("vk") && j["vk"].is_number())
        hk.vk = j["vk"].get<unsigned int>();
    return hk;
}

json HotkeyToJson(const HotkeyBinding& hk) {
    return json{{"modifiers", hk.modifiers}, {"vk", hk.vk}};
}

json PresetToJson(const ColorPreset& p) {
    return json{
        {"brightness", p.brightness},
        {"contrast", p.contrast},
        {"gamma", p.gamma},
        {"vibrance", p.vibrance},
        {"hue", p.hue}
    };
}

std::string GetExeDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string s(path);
    auto pos = s.find_last_of("\\/");
    return (pos != std::string::npos) ? s.substr(0, pos) : s;
}

} // anonymous namespace

namespace ConfigManager {

std::string GetConfigPath() {
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        std::string dir = std::string(appdata) + "\\ColorSwitcher";
        CreateDirectoryA(dir.c_str(), NULL);  // OK if already exists
        std::string path = dir + "\\config.json";
        // Test writability: try to open for append
        std::ofstream test(path, std::ios::app);
        if (test.good()) {
            test.close();
            return path;
        }
    }
    // Fallback: next to exe
    return GetExeDirectory() + "\\config.json";
}

AppConfig Load() {
    AppConfig config;
    std::string path = GetConfigPath();

    std::ifstream file(path);
    if (!file.is_open()) return config;  // Return defaults

    try {
        json j = json::parse(file);

        if (j.contains("version") && j["version"].is_number())
            config.version = j["version"].get<int>();

        if (j.contains("activePreset") && j["activePreset"].is_string()) {
            config.activePreset = j["activePreset"].get<std::string>();
            if (config.activePreset != "default" &&
                config.activePreset != "preset1" &&
                config.activePreset != "preset2") {
                config.activePreset = "default";
            }
        }

        if (j.contains("autoStart") && j["autoStart"].is_boolean())
            config.autoStart = j["autoStart"].get<bool>();

        if (j.contains("preset1") && j["preset1"].is_object())
            config.preset1 = ParsePreset(j["preset1"], config.preset1);

        if (j.contains("preset2") && j["preset2"].is_object())
            config.preset2 = ParsePreset(j["preset2"], config.preset2);

        if (j.contains("hotkeyDefault") && j["hotkeyDefault"].is_object())
            config.hotkeyDefault = ParseHotkey(j["hotkeyDefault"]);
        if (j.contains("hotkeyPreset1") && j["hotkeyPreset1"].is_object())
            config.hotkeyPreset1 = ParseHotkey(j["hotkeyPreset1"]);
        if (j.contains("hotkeyPreset2") && j["hotkeyPreset2"].is_object())
            config.hotkeyPreset2 = ParseHotkey(j["hotkeyPreset2"]);

    } catch (const json::exception& e) {
        OutputDebugStringA("ColorSwitcher: config parse error: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        config = AppConfig{};  // Reset to defaults
        Save(config);  // Recreate file with defaults
    }

    return config;
}

bool Save(const AppConfig& config) {
    json j;
    j["version"] = config.version;
    j["activePreset"] = config.activePreset;
    j["autoStart"] = config.autoStart;
    j["preset1"] = PresetToJson(config.preset1);
    j["preset2"] = PresetToJson(config.preset2);
    j["hotkeyDefault"] = HotkeyToJson(config.hotkeyDefault);
    j["hotkeyPreset1"] = HotkeyToJson(config.hotkeyPreset1);
    j["hotkeyPreset2"] = HotkeyToJson(config.hotkeyPreset2);

    std::string path = GetConfigPath();
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(2);
    return file.good();
}

const ColorPreset* GetActivePreset(const AppConfig& config) {
    if (config.activePreset == "preset1") return &config.preset1;
    if (config.activePreset == "preset2") return &config.preset2;
    return nullptr;  // "default" = identity ramp
}

} // namespace ConfigManager
