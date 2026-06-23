#include "PetFSM.h"

#include <Arduino.h>

#include <cstdlib>

void PetFSM::begin(Pet initial, State s) {
  // Seed the PRNG once from hardware entropy. esp_random() works via thermal
  // noise before any radio is initialised, so this is safe to call early.
  srand(esp_random());
  _pet = initial;
  _state = s;
  if (_stateCallback)
    _stateCallback(_state);
}

void PetFSM::transition(State next) {
  if (next == _state)
    return;
  _state = next;
  if (next == State::KEYBOARD)
    _pet.mood = (_pet.mood + 5 > 100) ? 100 : _pet.mood + 5;
  if (_stateCallback)
    _stateCallback(next);
}

void PetFSM::tick() {
  uint8_t randomStep = (uint8_t)(rand() % 5);

  switch (_state) {
  case State::KEYBOARD:
    if (_pet.energy > 0 && randomStep <= _pet.energy)
      _pet.energy -= randomStep;
    if (_pet.hunger + randomStep <= 100)
      _pet.hunger += randomStep;
    break;
  case State::EAT:
    if (_pet.hunger > 0 && randomStep <= _pet.hunger)
      _pet.hunger -= randomStep;
    if (_pet.hunger == 0)
      transition(State::IDLE);
    break;
  case State::PLAY:
    if (_pet.mood < 100 && _pet.mood + randomStep <= 100)
      _pet.mood += randomStep;
    if (_pet.energy > 0 && randomStep <= _pet.energy)
      _pet.energy -= randomStep;
    if (_pet.energy == 0)
      transition(State::IDLE);
    break;
  case State::SLEEP:
    if (_pet.energy + 2 <= 100)
      _pet.energy += 2;
    else
      _pet.energy = 100;
    break;
  case State::IDLE:
    if (_pet.hunger < 100)
      _pet.hunger++;
    break;
  }
}

void PetFSM::recoverFromSleep(uint32_t elapsed_ms, uint32_t tick_interval_ms) {
  if (tick_interval_ms == 0)
    return;
  uint32_t ticks = elapsed_ms / tick_interval_ms;
  uint32_t gain = ticks * 2; // same rate as SLEEP tick case
  uint32_t e = (uint32_t)_pet.energy + gain;
  _pet.energy = (uint8_t)(e > 100 ? 100 : e);
  // Do NOT call _stateCallback here — state is still SLEEP, firing it would
  // re-enter the sleep block in the subscriber. The caller transitions to IDLE
  // immediately after, which broadcasts the updated stats.
}

void PetFSM::handleGameInput(const InputEvent &) {}
