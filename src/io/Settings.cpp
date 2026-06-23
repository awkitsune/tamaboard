#include "Settings.h"

#include <Preferences.h>

static bool s_ledEnabled = true;
static bool s_airplaneMode = false;

void Settings::begin() {
  Preferences p;
  p.begin("settings", true);
  s_ledEnabled = p.getBool("led", true);
  s_airplaneMode = p.getBool("airplane", false);
  p.end();
}

bool Settings::ledEnabled() { return s_ledEnabled; }

void Settings::setLedEnabled(bool v) {
  s_ledEnabled = v;
  Preferences p;
  p.begin("settings", false);
  p.putBool("led", v);
  p.end();
}

bool Settings::airplaneMode() { return s_airplaneMode; }

void Settings::setAirplaneMode(bool v) {
  s_airplaneMode = v;
  Preferences p;
  p.begin("settings", false);
  p.putBool("airplane", v);
  p.end();
}
