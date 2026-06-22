#pragma once
#include "PetState.h"
#include "../Input.h"
#include "../hid/HidBackend.h"
#include <functional>

// Owns the pet's state machine and stats — nothing else. Side effects of being
// in a given state (HID lifecycle, WiFi power, light-sleep) are handled by the
// state-change subscriber, keeping this class focused on the "what" and the
// subscriber focused on the "how".
//
// DIP: depends on the HidBackend interface, not a concrete backend.
class PetFSM
{
public:
    explicit PetFSM(HidBackend &hid) : _hid(hid) {}

    void begin(Pet initial = {}, State s = State::IDLE);
    void tick(); // call every ~5 seconds
    void transition(State next);
    void handleGameInput(const InputEvent &e);
    void recoverFromSleep(uint32_t elapsed_ms, uint32_t tick_interval_ms);
    bool inKeyboardMode() const { return _state == State::KEYBOARD; }
    State state() const { return _state; }
    const Pet &pet() const { return _pet; }

    void onStateChange(std::function<void(State)> cb) { _stateCallback = cb; }

private:
    HidBackend &_hid;
    State _state = State::IDLE;
    Pet _pet;
    std::function<void(State)> _stateCallback;
};
