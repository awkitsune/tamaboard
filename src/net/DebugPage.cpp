#include "DebugPage.h"

#include <WiFi.h>

#include "../io/Battery.h"
#include "esp_system.h"

static const char *stateName(State s) {
  switch (s) {
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

static const char *bleStatusName(BleHidStatus s) {
  switch (s) {
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

static const char *resetReasonName() {
  switch (esp_reset_reason()) {
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

String buildDebugPage(PetFSM &fsm, BleHid &ble, DeviceSlots &slots) {
  uint32_t up = millis() / 1000;
  uint32_t h = up / 3600, m = (up % 3600) / 60, s = up % 60;
  const Pet &p = fsm.pet();
  char buf[2048];
  snprintf(
      buf, sizeof(buf),
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width'>"
      "<title>Tamaboard Debug</title>"
      "<style>body{font-family:monospace;background:#111;color:#ccc;padding:"
      "20px;max-width:480px}"
      "h1{color:#8f8;margin:0 0 16px}h2{color:#8af;font-size:.9em;margin:16px "
      "0 4px}"
      "table{border-collapse:collapse;width:100%%}td{padding:3px 10px 3px 0}"
      "td:first-child{color:#999;width:140px}a{color:#8af}</style></head>"
      "<body><h1><a href='/' "
      "style='color:#8f8;text-decoration:none'>Tamaboard</a></h1>"
      "<h2>System</h2><table>"
      "<tr><td>Uptime</td><td>%luh %02lum %02lus</td></tr>"
      "<tr><td>Free heap</td><td>%lu B</td></tr>"
      "<tr><td>Min free heap</td><td>%lu B</td></tr>"
      "<tr><td>WiFi IP</td><td>%s &nbsp;&middot;&nbsp; "
      "tamaboard.local</td></tr>"
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
      "<p style='margin-top:20px;font-size:.8em'><a "
      "href='/debug'>Refresh</a></p>"
      "</body></html>",
      (unsigned long)h, (unsigned long)m, (unsigned long)s,
      (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
      WiFi.softAPIP().toString().c_str(), resetReasonName(),
      stateName(fsm.state()), p.hunger, p.mood, p.energy,
      (unsigned)Battery::percent(), bleStatusName(ble.status()),
      (unsigned)slots.active());
  return String(buf);
}
