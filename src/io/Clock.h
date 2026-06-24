#pragma once
#include <cstddef>
#include <cstdint>

// Local-time clock. Synced by the companion via POST /api/clock — no NTP, no
// timezone math on-device. Before first sync we just show uptime as HH:MM.
namespace Clock {
void format(char *buf, size_t bufN);                // returns "HH:MM"
void syncFromLocalSeconds(uint32_t seconds_in_day); // 0..86399
bool synced();
} // namespace Clock
