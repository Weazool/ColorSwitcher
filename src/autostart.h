#pragma once

namespace AutoStart {
    // Check if the auto-start registry key exists.
    bool IsEnabled();

    // Enable auto-start: write exe path to Run key.
    // Returns true on success.
    bool Enable();

    // Disable auto-start: remove the Run key entry.
    // Returns true on success (or if already absent).
    bool Disable();
}
