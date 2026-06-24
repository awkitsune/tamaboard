#pragma once
#include "../Page.h"

// Modal shown when BLE pairing is in progress. Displays the 6-digit PIN big.
// Pushed by main.cpp via PageStack::pushModal when BleHid emits a passkey,
// popped on bond success/failure.
class PairingPage : public Page {
public:
  const char *title() override { return "PAIRING"; }
  void render(Canvas &c, int yTop) override;
  void onShortPress(PageStack &nav) override;
  void onLongPress(PageStack &nav) override;

  void setPasskey(uint32_t pk) { _pk = pk; }
  uint32_t passkey() const { return _pk; }

private:
  uint32_t _pk = 0;
};
