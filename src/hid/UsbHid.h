#pragma once
#include "HidBackend.h"

// Stub: GPIO19/20 native USB not wired on this board.
// Keeps the HidBackend seam intact for a possible future USB build.
class UsbHid : public HidBackend {
public:
  void begin() override;
  void end() override;
  void key(uint8_t keycode, bool down) override;
  void mouseMove(int8_t dx, int8_t dy) override;
  void mouseClick(uint8_t button) override;
};
