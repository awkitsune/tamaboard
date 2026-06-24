#pragma once
#include <cstdint>

#include "../Input.h"
#include "../pet/PetState.h"

class PetFSM;
class BleHid;
class DeviceSlots;

// Single command sink. Pages and the WebSocket dispatcher both call into this;
// nobody mutates state any other way. Decouples callers (UI, network) from
// concrete state owners (FSM, BLE backend, slots, clock).
class Controller {
public:
  Controller(PetFSM &fsm, BleHid &ble, DeviceSlots &slots, const char *apSsid,
             const char *apPass)
      : _fsm(fsm), _ble(ble), _slots(slots), _apSsid(apSsid), _apPass(apPass) {}

  // Pet
  void transitionState(State s);

  // BLE host slots
  enum class SlotAction : uint8_t { SELECT, PAIR, FORGET };
  void slotAction(uint8_t i, SlotAction action);

  // Clock sync from companion
  void syncClock(uint32_t local_seconds);

  // HID input — only honored while FSM is in KEYBOARD state
  void hidInput(const InputEvent &e);

  // Radio power — disables WiFi+BLE when true, restores AP when false.
  // BLE does NOT auto-restart on disable=false; use Keyboard page for that.
  void setAirplaneMode(bool enable);
  bool isAirplaneMode() const { return _airplaneMode; }

private:
  bool _airplaneMode = false;
  PetFSM &_fsm;
  BleHid &_ble;
  DeviceSlots &_slots;
  const char *_apSsid;
  const char *_apPass;
};
