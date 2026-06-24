#pragma once
#include <cstdint>

// Persistent user preferences (NVS namespace "settings").
// Read once at begin(); writes go to NVS immediately.
namespace Settings {
void begin();
bool ledEnabled();
void setLedEnabled(bool v);
bool airplaneMode();
void setAirplaneMode(bool v);
} // namespace Settings
