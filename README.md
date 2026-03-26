# ColorSwitcher

A lightweight Windows system tray utility for switching display color profiles on the primary monitor. Pure Win32 C++ with no runtime dependencies.

## Features

- **5 color adjustments** — Brightness, Contrast, Gamma, Digital Vibrance, and Hue
- **3 profiles** — Default (identity) + 2 customizable presets
- **System tray** — Right-click menu for instant profile switching
- **Preset editor** — GUI with sliders and real-time live preview
- **Global hotkeys** — Configurable keyboard shortcuts per profile
- **NVIDIA NVAPI** — Digital vibrance and hue via dynamic `nvapi64.dll` loading; gracefully disabled on non-NVIDIA systems
- **Auto-start** — Optional launch on Windows login
- **Single instance** — Prevents duplicate processes
- **Zero dependencies** — Statically linked, single portable `.exe`

## Build

Requires CMake 3.20+ and a C++17 compiler. Tested with MinGW (GCC via MSYS2).

```bash
mkdir build && cd build
cmake .. -G Ninja
cmake --build . --config Release
```

The resulting `ColorSwitcher.exe` is fully self-contained — no DLLs needed.

## Usage

1. Run `ColorSwitcher.exe`
2. An icon appears in the system tray
3. Right-click the icon:

```
  Preset 1            (active preset is checked)
  Preset 2
  Default
  ─────────────
  Customize Presets...
  Start with Windows
  ─────────────
  Quit
```

4. **Customize Presets** opens an editor with sliders for all 5 settings, plus hotkey assignment for each profile
5. Changes preview in real-time as you drag sliders
6. **Save** persists to disk; **Cancel** reverts

## Configuration

Stored at `%APPDATA%\ColorSwitcher\config.json` (created automatically on first launch).

```json
{
  "version": 1,
  "activePreset": "preset1",
  "autoStart": false,
  "preset1": {
    "brightness": 0.5,
    "contrast": 0.5,
    "gamma": 1.0,
    "vibrance": 50.0,
    "hue": 0.0
  },
  "preset2": {
    "brightness": 0.5,
    "contrast": 0.5,
    "gamma": 1.0,
    "vibrance": 50.0,
    "hue": 0.0
  },
  "hotkeyDefault": { "modifiers": 0, "vk": 0 },
  "hotkeyPreset1": { "modifiers": 0, "vk": 0 },
  "hotkeyPreset2": { "modifiers": 0, "vk": 0 }
}
```

## Project Structure

```
src/
  main.cpp              WinMain, message loop, tray icon, context menu
  gamma_engine.h/cpp    Gamma ramp calculation via SetDeviceGammaRamp
  nvapi_controller.h/cpp  NVIDIA vibrance & hue (dynamic NVAPI loading)
  config_manager.h/cpp  JSON config read/write (%APPDATA%)
  preset_editor.h/cpp   Editor dialog with sliders and hotkey controls
  autostart.h/cpp       Registry-based auto-start toggle
  resource.h            Control and message IDs
resources/
  app.rc                Resource script (icon, manifest)
  app.manifest          DPI-aware + Common Controls v6
  app.ico               Tray icon
third_party/
  nlohmann/json.hpp     JSON library (header-only, bundled)
```

## Technical Notes

- **Gamma ramp** is applied via `SetDeviceGammaRamp` on the primary display. Some environments (Remote Desktop, certain drivers) may silently reject extreme values.
- **NVAPI** is loaded dynamically from `nvapi64.dll` (installed by NVIDIA drivers). No SDK files are shipped. If no NVIDIA GPU is present, vibrance/hue sliders are grayed out but everything else works.
- **Display changes** and **wake from sleep** automatically re-apply the active preset (`WM_DISPLAYCHANGE`, `WM_POWERBROADCAST`).
- **Primary display only** — multi-monitor is not supported.

## License

MIT
