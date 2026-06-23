#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "esp_sleep.h"
#include "esp_task_wdt.h"

#include "Input.h"
#include "AppContext.h"
#include "app/Controller.h"
#include "io/Battery.h"
#include "io/Button.h"
#include "io/DeviceSlots.h"
#include "io/PetStore.h"
#include "hid/UsbHid.h"
#include "hid/BleHid.h"
#include "pet/PetFSM.h"
#include "net/WebUI.h"
#include "net/StateBroadcaster.h"
#include "ui/EinkDisplay.h"
#include "ui/PageStack.h"
#include "ui/Header.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/CarePage.h"
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
static WebUI webUI;

// Set in setup() once the dependents are constructed; held by pointer so
// callbacks can reach them.
static PageStack *pageStackPtr = nullptr;
static Controller *controllerPtr = nullptr;
static StateBroadcaster *broadcasterPtr = nullptr;
static PairingPage pairing;

static uint32_t lastTick = 0;
static int saveTick = 0;
static uint32_t lastClockRefresh = 0;
static uint32_t lastBatterySample = 0;
static uint32_t lastHomeRender = 0;

static const char *stateName(State s)
{
    switch (s)
    {
    case State::IDLE:
        return "IDLE";
    case State::SLEEP:
        return "SLEEP";
    case State::EAT:
        return "EAT";
    case State::PLAY:
        return "PLAY";
    case State::KEYBOARD:
        return "KEYBOARD";
    }
    return "?";
}

static const char *bleStatusName(BleHidStatus s)
{
    switch (s)
    {
    case BleHidStatus::IDLE:
        return "IDLE";
    case BleHidStatus::ADVERTISING:
        return "ADVERTISING";
    case BleHidStatus::PAIRING:
        return "PAIRING";
    case BleHidStatus::CONNECTED:
        return "CONNECTED";
    case BleHidStatus::FAILED:
        return "FAILED";
    }
    return "?";
}

static const char *resetReasonName()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON:
        return "Power-on";
    case ESP_RST_EXT:
        return "External pin";
    case ESP_RST_SW:
        return "Software";
    case ESP_RST_PANIC:
        return "Panic/crash";
    case ESP_RST_INT_WDT:
        return "Interrupt WDT";
    case ESP_RST_TASK_WDT:
        return "Task WDT";
    case ESP_RST_WDT:
        return "Other WDT";
    case ESP_RST_DEEPSLEEP:
        return "Deep sleep wake";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "Unknown";
    }
}

// ---- Side-effect runner: react to PetFSM state changes ----
//
// PetFSM stays pure; this listener does the dirty work: HID lifecycle on
// entering/leaving KEYBOARD, WiFi power-down and light-sleep on entering SLEEP.
// Centralizing here keeps the FSM testable and the side effects discoverable.
static void onFsmStateChange(State s)
{
    static State prev = State::IDLE;

    // Leaving KEYBOARD → tear down BLE.
    if (prev == State::KEYBOARD && s != State::KEYBOARD)
        bleHid.end();
    // Entering KEYBOARD → bring up BLE.
    if (s == State::KEYBOARD && prev != State::KEYBOARD)
        bleHid.begin();

    prev = s;
    headerSetLocked(s == State::SLEEP);
    headerSetSleeping(s == State::SLEEP);

    if (broadcasterPtr)
        broadcasterPtr->pushState();
    if (pageStackPtr)
        pageStackPtr->requestRender();

    if (s == State::SLEEP)
    {
        // Nav to Home root first so the sleep face always renders clean.
        pageStackPtr->goHome();
        // Tear down radios before rendering (cleaner timing, no BLE traffic during refresh).
        if (bleHid.isActive())
            bleHid.end();
        WiFi.mode(WIFI_OFF);
        // Commit the full-refresh sleep frame now, while the display is powered.
        pageStackPtr->renderIfDirty();
        petStore.save(fsm.pet(), State::SLEEP);
        Serial.println("[sleep] sleeping...");
        uint32_t sleepStart = millis();
        esp_light_sleep_start();
        // Awake. Reset home-render timer so it doesn't fire instantly.
        uint32_t sleptMs = millis() - sleepStart;
        Serial.printf("[sleep] waking up... slept %lu ms\n", (unsigned long)sleptMs);
        lastHomeRender = millis();
        fsm.recoverFromSleep(sleptMs, TICK_INTERVAL_MS);
        // ADC2 usable now (WiFi still off) — grab a fresh battery reading.
        Battery::sample();
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        // IDF keeps existing service registrations across sleep; begin() re-attaches
        // the daemon without clearing them, so addService() would fail here.
        MDNS.begin("tamaboard");
        Serial.println("[sleep] WiFi restored, transitioning to IDLE");
        controllerPtr->transitionState(State::IDLE);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(200); // give monitor time to attach
    esp_task_wdt_deinit(); // loopTask blocks intentionally during light sleep; TWDT adds no value here
    Serial.println("[main] boot");

    // SX1262 LoRa: never initialized — left in reset to save power.

    Serial.println("[main] mounting LittleFS...");
    if (!LittleFS.begin(true))
        Serial.println("[main] LittleFS mount failed");

    Serial.println("[main] loading slots...");
    slots.begin();
    Serial.println("[main] loading pet state...");
    petStore.begin();
    Serial.println("[main] battery init...");
    Battery::begin();
    Serial.println("[main] display init...");
    display.begin();
    Serial.println("[main] display ok");

    static PageStack stack(display);
    pageStackPtr = &stack;

    static Controller controller(fsm, bleHid, slots, AP_SSID, AP_PASS);
    controllerPtr = &controller;

    static StateBroadcaster broadcaster(webUI.websocket(), fsm, bleHid, slots);
    broadcasterPtr = &broadcaster;

    static AppContext ctx{fsm, bleHid, slots, controller, stack};

    static HomePage home(ctx);
    static KeyboardPage keyboard(ctx);
    static SettingsPage settings(ctx);
    stack.addRoot(&home);
    stack.addRoot(&keyboard);
    stack.addRoot(&settings);

    Serial.println("[main] button init...");
    button.begin();
    button.enableWake();
    button.onEvent([](ButtonEvent ev)
                   {
        if (ev == ButtonEvent::SHORT_PRESS) pageStackPtr->onShortPress();
        else                                pageStackPtr->onLongPress(); });

    // Wire state-change → broadcast + render.
    fsm.onStateChange(onFsmStateChange);
    bleHid.onStatus([](BleHidStatus)
                    {
        if (broadcasterPtr) { broadcasterPtr->pushState(); broadcasterPtr->pushSlots(); }
        if (pageStackPtr)   pageStackPtr->requestRender(); });
    bleHid.onPasskey([](uint32_t pk)
                     {
        pairing.setPasskey(pk);
        if (pk == 0) pageStackPtr->popModal();
        else         pageStackPtr->pushModal(&pairing);
        if (broadcasterPtr) broadcasterPtr->pushPairing(pk); });

    Serial.println("[main] WiFi softAP...");
    WiFi.softAP(AP_SSID, AP_PASS);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    MDNS.begin("tamaboard");
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[main] AP: %s  IP: %s  mDNS: tamaboard.local\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());

    webUI.setDebugProvider([&]() -> String
                           {
        uint32_t up = millis() / 1000;
        uint32_t h  = up / 3600, m = (up % 3600) / 60, s = up % 60;
        const Pet& p = fsm.pet();
        char buf[2048];
        snprintf(buf, sizeof(buf),
            "<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>Tamaboard Debug</title>"
            "<style>body{font-family:monospace;background:#111;color:#ccc;padding:20px;max-width:480px}"
            "h1{color:#8f8;margin:0 0 16px}h2{color:#8af;font-size:.9em;margin:16px 0 4px}"
            "table{border-collapse:collapse;width:100%%}td{padding:3px 10px 3px 0}"
            "td:first-child{color:#999;width:140px}a{color:#8af}</style></head>"
            "<body><h1><a href='/' style='color:#8f8;text-decoration:none'>Tamaboard</a></h1>"
            "<h2>System</h2><table>"
            "<tr><td>Uptime</td><td>%luh %02lum %02lus</td></tr>"
            "<tr><td>Free heap</td><td>%lu B</td></tr>"
            "<tr><td>Min free heap</td><td>%lu B</td></tr>"
            "<tr><td>WiFi IP</td><td>%s &nbsp;&middot;&nbsp; tamaboard.local</td></tr>"
            "<tr><td>Reset reason</td><td>%s</td></tr>"
            "</table><h2>Pet</h2><table>"
            "<tr><td>State</td><td>%s</td></tr>"
            "<tr><td>Hunger</td><td>%u / 100</td></tr>"
            "<tr><td>Mood</td><td>%u / 100</td></tr>"
            "<tr><td>Energy</td><td>%u / 100</td></tr>"
            "<tr><td>Battery</td><td>%u%%</td></tr>"
            "</table><h2>BLE</h2><table>"
            "<tr><td>Status</td><td>%s</td></tr>"
            "<tr><td>Active slot</td><td>%u</td></tr>"
            "</table>"
            "<p style='margin-top:20px;font-size:.8em'><a href='/debug'>Refresh</a></p>"
            "</body></html>",
            (unsigned long)h, (unsigned long)m, (unsigned long)s,
            (unsigned long)ESP.getFreeHeap(),
            (unsigned long)ESP.getMinFreeHeap(),
            WiFi.softAPIP().toString().c_str(),
            resetReasonName(),
            stateName(fsm.state()),
            p.hunger, p.mood, p.energy,
            (unsigned)Battery::percent(),
            bleStatusName(bleHid.status()),
            (unsigned)slots.active()
        );
        return String(buf); });

    Serial.println("[main] WebUI begin...");
    webUI.begin(controller, broadcaster);

    Serial.println("[main] FSM begin...");
    fsm.begin(petStore.pet(), petStore.state());
    Serial.println("[main] first render...");
    pageStackPtr->renderIfDirty();
    Serial.println("[main] setup complete");
    lastTick = millis();
}

void loop()
{
    button.poll();

    if (millis() - lastTick >= TICK_INTERVAL_MS)
    {
        lastTick = millis();
        fsm.tick();
        if (++saveTick >= 60)
        {
            saveTick = 0;
            petStore.save(fsm.pet(), fsm.state());
        }
    }
    if (millis() - lastClockRefresh >= 30000)
    {
        lastClockRefresh = millis();
        pageStackPtr->requestRender();
    }
    if (millis() - lastBatterySample >= 60000)
    {
        lastBatterySample = millis();
        Battery::sample();
        broadcasterPtr->pushState();
    }
    if (millis() - lastHomeRender >= 5000)
    {
        lastHomeRender = millis();
        if (fsm.state() != State::SLEEP && strcmp(pageStackPtr->currentPage()->title(), "HOME") == 0)
        {
            pageStackPtr->requestRender();
        }
    }

    pageStackPtr->renderIfDirty();
    delay(20);
}
