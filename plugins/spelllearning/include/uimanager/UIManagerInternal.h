#pragma once

#include "Common.h"

// =============================================================================
// Internal helpers shared across UIManager implementation files.
// NOT part of the public API — only included by UIManager*.cpp files.
// =============================================================================

// nlohmann::json::value() throws type_error.306 when key exists but is null.
// This helper safely returns the default if the key is missing OR null.
template<typename T>
T SafeJsonValue(const nlohmann::json& j, const std::string& key, const T& defaultValue) {
    if (j.contains(key) && !j[key].is_null()) {
        try {
            return j[key].get<T>();
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

// Forward declaration for InputHandler access (defined in Main.cpp)
void UpdateInputHandlerHotkey(uint32_t keyCode);

// Forward declaration for controller hotkey combo access (defined in Main.cpp)
void UpdateControllerHotkey(uint32_t primary, uint32_t secondary);

// Forward declaration for controller button capture (defined in Main.cpp)
// Capture is a polling mechanism: BeginControllerCapture arms the InputHandler,
// the next gamepad button press is stored, and GetCapturedCode() returns it.
void BeginControllerCapture(bool primary);
void EndControllerCapture();
uint32_t GetCapturedCode();
