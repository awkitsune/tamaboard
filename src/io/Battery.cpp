#include "Battery.h"

#include <Arduino.h>
#include <WiFi.h>

static constexpr int BATTERY_PIN = 20; // ADC2
static constexpr int ADC_CTRL = 19;    // active-high enable for the divider

// LiPo discharge curve approximated as linear between empty and full.
// Tweak after measuring on hardware.
static constexpr int VBAT_EMPTY_MV = 3300;
static constexpr int VBAT_FULL_MV = 4200;

// 1:1 voltage divider → battery_mv = pin_mv * 2.
// Meshtastic firmware confirms ADC_MULTIPLIER = 2 for this board.
static constexpr int DIVIDER_NUM = 2;
static constexpr int DIVIDER_DEN = 1;

static uint8_t _cached = 100;

namespace Battery {

void begin() {
  pinMode(ADC_CTRL, OUTPUT);
  digitalWrite(ADC_CTRL,
               HIGH); // HIGH = divider disabled (active-LOW gate, P-FET)
  sample();
}

void sample() {
  // ESP32-S3 ADC2 returns invalid data while WiFi is active. Skip the read
  // and let the last good value stick rather than clobber it with 0.
  // Callers that need a fresh value should bring WiFi down briefly first
  // (we do this implicitly on wake-from-sleep in main.cpp).
  if (WiFi.getMode() != WIFI_OFF)
    return;

  digitalWrite(ADC_CTRL,
               LOW); // LOW = enable divider (active-LOW, matches Meshtastic)
  delayMicroseconds(200); // settle
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++)
    sum += analogReadMilliVolts(BATTERY_PIN);
  digitalWrite(ADC_CTRL, HIGH); // disable divider between samples

  int pin_mv = sum / 8;
  int vbat = pin_mv * DIVIDER_NUM / DIVIDER_DEN;

  if (vbat <= VBAT_EMPTY_MV)
    _cached = 0;
  else if (vbat >= VBAT_FULL_MV)
    _cached = 100;
  else
    _cached = (uint8_t)((vbat - VBAT_EMPTY_MV) * 100 /
                        (VBAT_FULL_MV - VBAT_EMPTY_MV));

  Serial.printf("[bat] %d mV -> %u%%\n", vbat, _cached);
}

uint8_t percent() { return _cached; }

} // namespace Battery
