#pragma once
#include <cstdint>

enum class InputType : uint8_t { KEY, MOVE, CLICK };

struct InputEvent {
    InputType type;
    // KEY
    uint8_t code;
    bool    down;
    // MOVE
    int8_t dx;
    int8_t dy;
    // CLICK
    uint8_t button;
};
