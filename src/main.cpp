#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "AppContext.h"
#include "Input.h"
#include "app/Controller.h"
#include "esp_sleep.h"
#include "hid/BleHid.h"
#include "hid/UsbHid.h"
#include "io/Battery.h"
#include "io/Button.h"
#include "io/DeviceSlots.h"
#include "io/Led.h"
#include "io/PetStore.h"
#include "io/Settings.h"
#include "net/DebugPage.h"
#include "net/StateBroadcaster.h"
#include "net/WebUI.h"
#include "pet/PetFSM.h"
#include "routine/BatteryRoutine.h"
#include "routine/RoutineManager.h"
#include "ui/EinkDisplay.h"
#include "ui/Header.h"
#include "ui/PageStack.h"
#include "ui/pages/CarePage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/KeyboardPage.h"
#include "ui/pages/PairingPage.h"
#include "ui/pages/SettingsPage.h"

static constexpr char AP_SSID[] = "Tamaboard";
static constexpr char AP_PASS[] = "tamaboard";
static constexpr uint32_t TICK_INTERVAL_MS = 5000;

static DeviceSlots slots;
static PetStore petStore;
static UsbHid usbHid;
static BleHid bleHid(slots);
static EinkDisplay display;
static PetFSM fsm(bleHid); // FSM holds HidBackend& — DIP
static Button button;
static Led led;
static WebUI webUI;
static RoutineManager routines;

// Set in setup() once the dependents are constructed; held by pointer so
// callbacks can reach them.
static PageStack *pageStackPtr = nullptr;
static Controller *controllerPtr = nullptr;
static StateBroadcaster *broadcasterPtr = nullptr;
static PairingPage pairing;

static uint32_t lastTick = 0;
static int saveTick = 0;

static void startWiFiAP() {
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  MDNS.begin("tamaboard");
  MDNS.addService("http", "tcp", 80);
  Serial.printf("[main] AP: %s  IP: %s  mDNS: tamaboard.local\n", AP_SSID,
                WiFi.softAPIP().toString().c_str());
}

// IDF keeps existing mDNS service registrations across sleep; begin()
// re-attaches the daemon without clearing them, so addService() must not be
// called here.
static void restoreWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  MDNS.begin("tamaboard");
  Serial.println("[sleep] WiFi restored");
}

// ---- Side-effect runner: react to PetFSM state changes ----
//
// PetFSM stays pure; this listener does the dirty work: HID lifecycle on
// entering/leaving KEYBOARD, WiFi power-down and light-sleep on entering SLEEP.
// Centralizing here keeps the FSM testable and the side effects discoverable.
static void onFsmStateChange(State s) {
  static State prev = State::IDLE;

  // Leaving KEYBOARD → tear down BLE.
  if (prev == State::KEYBOARD && s != State::KEYBOARD)
    bleHid.end();
  // Entering KEYBOARD → bring up BLE.
  if (s == State::KEYBOARD && prev != State::KEYBOARD)
    bleHid.begin();

  prev = s;
  headerSetLocked(s == State::SLEEP);
  headerSetCustomText(s == State::SLEEP ? "SLEEP" : nullptr);
  led.play(LedPattern::SINGLE);

  if (broadcasterPtr)
    broadcasterPtr->pushState();
  if (pageStackPtr)
    pageStackPtr->requestRender();

  if (s == State::SLEEP) {
    // Nav to Home root first so the sleep face always renders clean.
    pageStackPtr->goHome();
    // Tear down radios before rendering (cleaner timing, no BLE traffic during
    // refresh).
    if (bleHid.isActive())
      bleHid.end();
    WiFi.mode(WIFI_OFF);
    // WiFi.mode() yields internally — loopTask may have consumed _dirty by now.
    // Re-mark dirty immediately before renderIfDirty() so there is no yield
    // opportunity between the two calls and loopTask cannot steal this render.
    pageStackPtr->requestRender();
    pageStackPtr->renderIfDirty();
    petStore.save(fsm.pet(), State::SLEEP);
    Serial.println("[sleep] sleeping...");
    uint32_t sleepStart = millis();
    disableLoopWDT();
    esp_light_sleep_start();
    // Swallow the wakeup button press before _taskFn can turn it into a nav
    // event.
    button.suppressNextEvent();
    // Wake-based full refresh: e-ink holds the sleep face; clear artifacts
    // cleanly.
    pageStackPtr->markFullRefresh();
    uint32_t sleptMs = millis() - sleepStart;
    Serial.printf("[sleep] waking up... slept %lu ms\n",
                  (unsigned long)sleptMs);
    fsm.recoverFromSleep(sleptMs, TICK_INTERVAL_MS);
    // ADC2 usable now (WiFi still off) — grab a fresh battery reading.
    Battery::sample();
    if (!controllerPtr->isAirplaneMode())
      restoreWiFiAP();
    else
      Serial.println("[sleep] airplane mode active — WiFi stays off");
    Serial.println("[sleep] transitioning to IDLE");
    controllerPtr->transitionState(State::IDLE);
    petStore.save(fsm.pet(),
                  State::IDLE); // persist IDLE immediately so a crash here
                                // doesn't reboot into SLEEP
    enableLoopWDT();            // re-subscribe after all slow wake ops complete
  }
}

void setup() {
  Serial.begin(115200);
  delay(200); // give monitor time to attach
  Serial.println("[main] boot");

  // SX1262 LoRa: never initialized — left in reset to save power.

  if (!LittleFS.begin(true))
    Serial.println("[main] LittleFS mount failed");

  Settings::begin();
  slots.begin();
  petStore.begin();
  Battery::begin();
  display.begin();

  static PageStack stack(display);
  pageStackPtr = &stack;

  static Controller controller(fsm, bleHid, slots, AP_SSID, AP_PASS);
  controllerPtr = &controller;

  static StateBroadcaster broadcaster(webUI.websocket(), fsm, bleHid, slots);
  broadcasterPtr = &broadcaster;

  led.setEnabled(Settings::ledEnabled());

  static AppContext ctx{fsm, bleHid, slots, controller, stack, led};

  static HomePage home(ctx);
  static KeyboardPage keyboard(ctx);
  static SettingsPage settings(ctx);
  stack.addRoot(&home);
  stack.addRoot(&keyboard);
  stack.addRoot(&settings);

  button.begin();
  button.enableWake();
  button.onEvent([](ButtonEvent ev) {
    if (ev == ButtonEvent::SHORT_PRESS)
      pageStackPtr->onShortPress();
    else
      pageStackPtr->onLongPress();
  });

  // Wire state-change → broadcast + render.
  fsm.onStateChange(onFsmStateChange);
  bleHid.onStatus([](BleHidStatus status) {
    if (broadcasterPtr) {
      broadcasterPtr->pushState();
      broadcasterPtr->pushSlots();
    }
    if (pageStackPtr)
      pageStackPtr->requestRender();
    switch (status) {
    case BleHidStatus::ADVERTISING:
      led.play(LedPattern::SINGLE);
      break;
    case BleHidStatus::CONNECTED:
      led.play(LedPattern::DOUBLE);
      break;
    case BleHidStatus::FAILED:
      led.play(LedPattern::TRIPLE);
      break;
    default:
      break;
    }
  });
  bleHid.onPasskey([](uint32_t pk) {
    pairing.setPasskey(pk);
    if (pk == 0)
      pageStackPtr->popModal();
    else {
      pageStackPtr->pushModal(&pairing);
      led.play(LedPattern::FAST);
    }
    if (broadcasterPtr)
      broadcasterPtr->pushPairing(pk);
  });

  if (Settings::airplaneMode()) {
    controller.setAirplaneMode(true);
    headerSetAirplaneMode(true);
  } else {
    startWiFiAP();
  }

  webUI.setDebugProvider(
      [&]() -> String { return buildDebugPage(fsm, bleHid, slots); });

  pageStackPtr->setPauseTimeout(30000); // pause e-ink after 30 s idle

  routines.add(&led);
  static BatteryRoutine batteryRoutine(broadcaster);
  routines.add(&batteryRoutine);

  webUI.begin(controller, broadcaster);
  fsm.begin(petStore.pet(), petStore.state());
  pageStackPtr->renderIfDirty();
  Serial.println("[main] setup complete");
  lastTick = millis();
}

void loop() {
  if (millis() - lastTick >= TICK_INTERVAL_MS) {
    lastTick = millis();
    fsm.tick();
    if (++saveTick >= 60) {
      saveTick = 0;
      petStore.save(fsm.pet(), fsm.state());
    }
  }
  // Auto-pause: stop e-ink updates after idle timeout.
  if (pageStackPtr->pauseTimeoutMs() > 0 && !pageStackPtr->isScreenPaused() &&
      millis() - pageStackPtr->lastActivityMs() >=
          pageStackPtr->pauseTimeoutMs()) {
    pageStackPtr->setScreenPaused(true);
  }

  routines.tick();
  pageStackPtr->checkAutoRefresh();
  pageStackPtr->renderIfDirty();
  delay(20);
}
