#pragma once
#include <cstdint>

struct HidBackend {
    virtual void begin() = 0;
    virtual void end()   = 0;
    virtual void key(uint8_t keycode, bool down)   = 0;
    virtual void mouseMove(int8_t dx, int8_t dy)   = 0;
    virtual void mouseClick(uint8_t button)         = 0;
    virtual ~HidBackend() = default;
};
