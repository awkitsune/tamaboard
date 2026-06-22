#pragma once
#include <cstdint>

enum class State : uint8_t { IDLE, SLEEP, EAT, PLAY, KEYBOARD };

struct Pet {
    uint8_t hunger = 50;
    uint8_t mood   = 50;
    uint8_t energy = 100;
};
