#include "Button.h"
#include <Arduino.h>
#include "esp_sleep.h"

static constexpr uint32_t LONG_PRESS_MS  = 800;
static constexpr uint32_t DEBOUNCE_MS    = 30;

void Button::begin() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void Button::poll() {
    bool raw = (digitalRead(BUTTON_PIN) == LOW);
    uint32_t now = millis();

    if (raw && !_pressed) {
        // Debounce leading edge.
        if (now - _pressStart < DEBOUNCE_MS) return;
        _pressed    = true;
        _pressStart = now;
    } else if (!raw && _pressed) {
        uint32_t held = now - _pressStart;
        _pressed = false;
        if (held < DEBOUNCE_MS) return;
        if (_callback)
            _callback(held >= LONG_PRESS_MS ? ButtonEvent::LONG_PRESS
                                            : ButtonEvent::SHORT_PRESS);
    }
}

void Button::enableWake() {
    // Wake from light sleep when the button is pressed (GPIO goes LOW).
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
}
