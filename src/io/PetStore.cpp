#include "PetStore.h"
#include <Preferences.h>

static Preferences prefs;

void PetStore::begin() {
    prefs.begin("pet", false);
    _pet.hunger = prefs.getUChar("hunger", 50);
    _pet.mood   = prefs.getUChar("mood",   50);
    _pet.energy = prefs.getUChar("energy", 100);
    uint8_t raw = prefs.getUChar("state", (uint8_t)State::IDLE);
    // All non-IDLE states reset to IDLE on reboot — SLEEP especially must not
    // persist across a physical reset (user explicitly powered off or crashed).
    State s = static_cast<State>(raw);
    _state = (s == State::IDLE) ? s : State::IDLE;
    prefs.end();
}

void PetStore::save(const Pet& p, State s) {
    prefs.begin("pet", false);
    prefs.putUChar("hunger", p.hunger);
    prefs.putUChar("mood",   p.mood);
    prefs.putUChar("energy", p.energy);
    prefs.putUChar("state",  (uint8_t)s);
    prefs.end();
}
