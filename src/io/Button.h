#pragma once
#include <cstdint>
#include <functional>

// PRG button on GPIO0 (active-low, internal pull-up).
// Verify against board pinout before relying on this.
static constexpr uint8_t BUTTON_PIN = 0;

enum class ButtonEvent { SHORT_PRESS, LONG_PRESS };

class Button {
public:
    void begin();

    // Call from loop(). Fires callback on press events.
    void poll();

    void onEvent(std::function<void(ButtonEvent)> cb) { _callback = cb; }

    // Configure this pin as the light-sleep wake source.
    void enableWake();

private:
    uint32_t _pressStart = 0;
    bool     _pressed    = false;

    std::function<void(ButtonEvent)> _callback;
};
