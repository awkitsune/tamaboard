#include "Button.h"
#include <Arduino.h>
#include "esp_sleep.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static constexpr uint32_t LONG_PRESS_MS = 800;
static constexpr uint32_t DEBOUNCE_MS   = 30;

static QueueHandle_t   s_queue    = nullptr;
static volatile bool   s_suppress = false;

// ISR must be a plain function (IRAM_ATTR + static member = compiler grief).
static void IRAM_ATTR buttonIsr() {
    BaseType_t woken = pdFALSE;
    uint8_t dummy = 0;
    xQueueSendFromISR(s_queue, &dummy, &woken);
    if (woken) portYIELD_FROM_ISR();
}

void Button::_taskFn(void* arg) {
    Button* self = static_cast<Button*>(arg);
    Serial.println("[button] task started on core 0");

    bool     pressing   = false;
    uint32_t pressStart = 0;
    uint8_t  dummy;

    while (true) {
        // Block until any edge fires the ISR.
        xQueueReceive(s_queue, &dummy, portMAX_DELAY);

        // Drain rapid bounces: consume all further edges within the debounce window.
        while (xQueueReceive(s_queue, &dummy, pdMS_TO_TICKS(DEBOUNCE_MS)));

        // Read settled GPIO state.
        bool isPressed = (digitalRead(BUTTON_PIN) == LOW);
        uint32_t now   = millis();

        if (isPressed && !pressing) {
            pressing   = true;
            pressStart = now;
        } else if (!isPressed && pressing) {
            uint32_t held = now - pressStart;
            pressing = false;
            if (s_suppress) {
                s_suppress = false;
            } else if (self->_callback) {
                self->_callback(held >= LONG_PRESS_MS ? ButtonEvent::LONG_PRESS
                                                      : ButtonEvent::SHORT_PRESS);
            }
        }
    }
}

void Button::begin() {
    s_queue = xQueueCreate(8, sizeof(uint8_t));
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonIsr, CHANGE);
    // 8 KB: callback chain runs onFsmStateChange → WiFi.mode() + esp_light_sleep_start(),
    // both of which are stack-heavy (WiFi driver uses vsnprintf internally).
    xTaskCreatePinnedToCore(_taskFn, "button", 8192, this, 5, nullptr, 0);
}

void Button::enableWake() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
}

void Button::suppressNextEvent() {
    s_suppress = true;
}
