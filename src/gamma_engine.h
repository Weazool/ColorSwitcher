#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ColorPreset {
    float brightness = 0.5f;  // 0.0 - 1.0
    float contrast = 0.5f;    // 0.0 - 1.0
    float gamma = 1.0f;       // 0.1 - 5.0
    float vibrance = 50.0f;   // 0 - 100 (NVAPI DVC, 50 = neutral)
    float hue = 0.0f;         // 0 - 359 degrees (0 = no shift)
};

namespace GammaEngine {
    // Calculate and apply a gamma ramp from preset values.
    // Returns true if SetDeviceGammaRamp succeeded.
    bool ApplyPreset(const ColorPreset& preset);

    // Apply identity gamma ramp (no adjustments).
    bool ApplyDefault();

    // Build a gamma ramp from preset values without applying it.
    // ramp must point to a WORD[3][256] array (same layout as SetDeviceGammaRamp).
    void BuildRamp(const ColorPreset& preset, WORD ramp[3][256]);
}
