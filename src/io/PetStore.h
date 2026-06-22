#pragma once
#include "../pet/PetState.h"

// Persists pet stats to NVS (Preferences namespace "pet").
// PetFSM stays pure — load before fsm.begin(), save from main.cpp.
class PetStore {
public:
    void  begin();
    Pet   pet()   const { return _pet; }
    State state() const { return _state; }
    void  save(const Pet& p, State s);
private:
    Pet   _pet;
    State _state = State::IDLE;
};
