#pragma once

namespace NvapiController {
    // Load nvapi64.dll, resolve functions, initialize, enumerate primary display.
    // Returns true if NVAPI is available and ready.
    bool Init();

    // True if Init() succeeded and NVAPI functions are available.
    bool IsAvailable();

    // Set digital vibrance level (0-100, 50 = neutral/default).
    void SetVibrance(int level);

    // Set hue angle in degrees (0-359, 0 = no shift/default).
    void SetHue(int angle);

    // Reset vibrance and hue to defaults (vibrance=50, hue=0).
    void ResetDefaults();

    // Unload NVAPI. Call at application exit.
    void Shutdown();
}
