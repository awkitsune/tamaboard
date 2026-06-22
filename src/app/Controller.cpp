#include "Controller.h"
#include "../pet/PetFSM.h"
#include "../hid/BleHid.h"
#include "../io/DeviceSlots.h"
#include "../io/Clock.h"
#include "../io/EventLog.h"
#include <WiFi.h>

void Controller::transitionState(State s) {
    if (s == _fsm.state()) return;          // ignore self-transitions
    _fsm.transition(s);
    switch (s) {
        case State::EAT:      EventLog::add("Fed");         break;
        case State::PLAY:     EventLog::add("Played");      break;
        case State::SLEEP:    EventLog::add("Sleep");       break;
        case State::KEYBOARD: EventLog::add("Keyboard on"); break;
        case State::IDLE:     EventLog::add("Idle");        break;
    }
}

void Controller::slotAction(uint8_t i, SlotAction action) {
    if (i >= SLOT_COUNT) return;
    switch (action) {
        case SlotAction::SELECT:
        case SlotAction::PAIR:
            // BLE must be initialised before advertising. Mirror what KeyboardPage
            // does on-device: transition to KEYBOARD first (fires bleHid.begin()
            // via onFsmStateChange), then execute the slot action.
            transitionState(State::KEYBOARD);
            if (action == SlotAction::SELECT) _ble.selectSlot(i);
            else                              _ble.pairInto(i);
            break;
        case SlotAction::FORGET:
            _ble.forgetSlot(i);
            break;
    }
}

void Controller::syncClock(uint32_t local_seconds) {
    Clock::syncFromLocalSeconds(local_seconds);
}

void Controller::hidInput(const InputEvent& e) {
    if (!_fsm.inKeyboardMode()) return;
    switch (e.type) {
        case InputType::KEY:   _ble.key(e.code, e.down);   break;
        case InputType::MOVE:  _ble.mouseMove(e.dx, e.dy); break;
        case InputType::CLICK: _ble.mouseClick(e.button);  break;
    }
}

void Controller::setAirplaneMode(bool enable) {
    if (enable) {
        if (_ble.isActive()) _ble.end();
        WiFi.mode(WIFI_OFF);
    } else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(_apSsid, _apPass);
    }
}
