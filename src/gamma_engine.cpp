#include "gamma_engine.h"
#include "nvapi_controller.h"
#include <cmath>
#include <algorithm>

namespace GammaEngine {

void BuildRamp(const ColorPreset& preset, WORD ramp[3][256]) {
    for (int i = 0; i < 256; i++) {
        double t = i / 255.0;

        // Apply gamma
        t = pow(t, 1.0 / static_cast<double>(preset.gamma));

        // Apply contrast
        t = (t - 0.5) * (static_cast<double>(preset.contrast) * 2.0) + 0.5;

        // Apply brightness
        t = t + (static_cast<double>(preset.brightness) - 0.5);

        // Clamp
        t = std::clamp(t, 0.0, 1.0);

        // Scale to 16-bit
        WORD value = static_cast<WORD>(t * 65535.0);
        ramp[0][i] = value;  // Red
        ramp[1][i] = value;  // Green
        ramp[2][i] = value;  // Blue
    }
}

bool ApplyPreset(const ColorPreset& preset) {
    HDC hdc = GetDC(NULL);
    if (!hdc) return false;

    WORD ramp[3][256];
    BuildRamp(preset, ramp);

    BOOL result = SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);

    // Apply NVAPI settings (vibrance, hue)
    NvapiController::SetVibrance(static_cast<int>(preset.vibrance));
    NvapiController::SetHue(static_cast<int>(preset.hue));

    return result != FALSE;
}

bool ApplyDefault() {
    HDC hdc = GetDC(NULL);
    if (!hdc) return false;

    WORD ramp[3][256];
    for (int i = 0; i < 256; i++) {
        WORD value = static_cast<WORD>(i * 257);
        ramp[0][i] = value;
        ramp[1][i] = value;
        ramp[2][i] = value;
    }

    BOOL result = SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);

    // Reset NVAPI to defaults
    NvapiController::ResetDefaults();

    return result != FALSE;
}

} // namespace GammaEngine
