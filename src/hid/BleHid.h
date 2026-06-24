#pragma once
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <functional>

#include "../io/DeviceSlots.h"
#include "HidBackend.h"

enum class BleHidStatus : uint8_t {
  IDLE,
  ADVERTISING,
  PAIRING, // waiting for passkey confirmation
  CONNECTED,
  FAILED,
};

class BleHid : public HidBackend, private NimBLEServerCallbacks {
public:
  explicit BleHid(DeviceSlots &slots);

  void begin() override;
  void end() override;
  void key(uint8_t keycode, bool down) override;
  void mouseMove(int8_t dx, int8_t dy) override;
  void mouseClick(uint8_t button) override;

  // Switch to a bonded slot (disconnect current, reconnect to slot peer).
  void selectSlot(uint8_t i);
  // Pair a new host into slot i (undoes existing bond for that slot).
  void pairInto(uint8_t i);
  // Remove bond for slot i.
  void forgetSlot(uint8_t i);

  BleHidStatus status() const { return _status; }
  bool isActive() const { return _active; }

  // Called from the main task — fires with passkey during pairing,
  // and with passkey=0 on bond success/failure.
  void onPasskey(std::function<void(uint32_t passkey)> cb) {
    _passkeyCallback = cb;
  }
  void onStatus(std::function<void(BleHidStatus)> cb) { _statusCallback = cb; }

private:
  // NimBLEServerCallbacks
  void onConnect(NimBLEServer *srv, NimBLEConnInfo &info) override;
  void onDisconnect(NimBLEServer *srv, NimBLEConnInfo &info,
                    int reason) override;
  void onAuthenticationComplete(NimBLEConnInfo &info) override;
  uint32_t onPassKeyDisplay() override;

  void _startAdvertising();
  void _setStatus(BleHidStatus s);

  DeviceSlots &_slots;
  NimBLEServer *_server = nullptr;
  NimBLEHIDDevice *_hid = nullptr;
  NimBLECharacteristic *_keyReport = nullptr;
  NimBLECharacteristic *_mouseReport = nullptr;

  BleHidStatus _status = BleHidStatus::IDLE;
  uint8_t _pairingSlot = 0; // slot currently being paired into
  uint8_t _modifiers = 0;   // live modifier byte (L-Ctrl/Shift/Alt/GUI bits)
  bool _active = false;

  std::function<void(uint32_t)> _passkeyCallback;
  std::function<void(BleHidStatus)> _statusCallback;
};
