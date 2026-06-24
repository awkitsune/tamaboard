#pragma once
#include <cstdint>
#include <functional>

// PRG button on GPIO0 (active-low, internal pull-up).
static constexpr uint8_t BUTTON_PIN = 0;

enum class ButtonEvent { SHORT_PRESS, LONG_PRESS };

class Button {
public:
  // Sets up GPIO, attaches interrupt, and starts debounce task on Core 0.
  void begin();

  void onEvent(std::function<void(ButtonEvent)> cb) { _callback = cb; }

  // Configure this pin as the light-sleep wake source.
  void enableWake();

  // Swallow the next button event — call immediately after
  // esp_light_sleep_start() returns to discard the wakeup press before it
  // reaches the navigation callback.
  void suppressNextEvent();

private:
  static void _taskFn(void *arg); // IRAM_ATTR only on definition in .cpp

  std::function<void(ButtonEvent)> _callback;
};
