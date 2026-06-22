#pragma once
#include <cstdint>

// Heltec Wireless Paper v1.2 battery sensing (pin assignment matches Meshtastic):
//   BATTERY_PIN = GPIO20  (ADC2 channel, ~1:1 divider)
//   ADC_CTRL    = GPIO19  (active-high gate, so the divider only sinks current
//                          during measurement)
// Note: ADC2 reads can be unreliable while WiFi/BT radios are active on ESP32-S3.
// We sample infrequently and accept occasional bad reads (filtered by callers).
namespace Battery {
    void    begin();
    uint8_t percent();         // last cached SOC, 0..100
    void    sample();           // do one ADC read, update cached SOC
}
