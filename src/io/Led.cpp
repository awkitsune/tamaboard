#include "Led.h"

#include <Arduino.h>

// Each array is alternating on/off durations in ms.
// Even indices (0, 2, …) = LED on; odd = LED off.
static const uint16_t PAT_SINGLE[] = {80};
static const uint16_t PAT_DOUBLE[] = {80, 80, 80};
static const uint16_t PAT_TRIPLE[] = {80, 80, 80, 80, 80};
static const uint16_t PAT_FAST[] = {40, 40, 40, 40, 40, 40, 40, 40};

void Led::begin() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void Led::play(LedPattern p) {
  if (!_enabled)
    return;
  switch (p) {
  case LedPattern::SINGLE:
    _steps = PAT_SINGLE;
    _stepCount = 1;
    break;
  case LedPattern::DOUBLE:
    _steps = PAT_DOUBLE;
    _stepCount = 3;
    break;
  case LedPattern::TRIPLE:
    _steps = PAT_TRIPLE;
    _stepCount = 5;
    break;
  case LedPattern::FAST:
    _steps = PAT_FAST;
    _stepCount = 8;
    break;
  }
  _stepIdx = 0;
  _elapsed = 0;
  digitalWrite(LED_PIN, HIGH); // step 0 always starts on
}

void Led::setEnabled(bool v) {
  _enabled = v;
  if (!v) {
    _stepCount = 0;
    digitalWrite(LED_PIN, LOW);
  }
}

void Led::tick() {
  if (!_enabled || _stepCount == 0)
    return;

  _elapsed += intervalMs();
  if (_elapsed < _steps[_stepIdx])
    return;

  _elapsed -= _steps[_stepIdx];
  _stepIdx++;

  if (_stepIdx >= _stepCount) {
    _stepCount = 0;
    digitalWrite(LED_PIN, LOW);
    return;
  }

  // Even step index = on, odd = off
  digitalWrite(LED_PIN, (_stepIdx % 2 == 0) ? HIGH : LOW);
}
