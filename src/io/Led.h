#pragma once
#include <cstdint>

#include "../routine/Routine.h"

static constexpr uint8_t LED_PIN =
    18; // active-HIGH, confirmed on Wireless Paper v1.2

enum class LedPattern {
  SINGLE, // 1 flash  — generic event / state change
  DOUBLE, // 2 flashes — BLE connected
  TRIPLE, // 3 flashes — BLE failed
  FAST,   // 4 rapid   — pairing in progress
};

class Led : public Routine {
public:
  void begin() override;
  void tick() override;
  uint32_t intervalMs() const override { return 20; }

  void play(LedPattern p);
  void setEnabled(bool v);
  bool enabled() const { return _enabled; }

private:
  bool _enabled = true;
  const uint16_t *_steps = nullptr;
  uint8_t _stepCount = 0;
  uint8_t _stepIdx = 0;
  uint32_t _elapsed = 0;
};
