#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

static constexpr uint8_t SLOT_COUNT = 3;

struct DeviceSlot {
  bool occupied = false;
  char name[24] = {};
  NimBLEAddress addr;
};

class DeviceSlots {
public:
  void begin();
  void save();

  DeviceSlot &operator[](uint8_t i) { return _slots[i]; }
  const DeviceSlot &operator[](uint8_t i) const { return _slots[i]; }

  uint8_t active() const { return _active; }
  void setActive(uint8_t i);

  // Find slot index by peer address, -1 if not found.
  int findByAddr(const NimBLEAddress &addr) const;

private:
  DeviceSlot _slots[SLOT_COUNT];
  uint8_t _active = 0;
};
