#include "Clock.h"

#include <Arduino.h>

#include <cstdio>

namespace Clock {

static uint32_t _baseLocal = 0; // seconds-since-midnight at sync time
static uint32_t _baseMillis = 0;
static bool _synced = false;

void syncFromLocalSeconds(uint32_t s) {
  _baseLocal = s % 86400UL;
  _baseMillis = millis();
  _synced = true;
}

bool synced() { return _synced; }

void format(char *buf, size_t bufN) {
  uint32_t s;
  if (_synced) {
    uint32_t elapsed = (millis() - _baseMillis) / 1000UL;
    s = (_baseLocal + elapsed) % 86400UL;
  } else {
    s = (millis() / 1000UL) % 86400UL;
  }
  snprintf(buf, bufN, "%02u:%02u", (unsigned)(s / 3600),
           (unsigned)((s / 60) % 60));
}

} // namespace Clock
