#include "SettingsPage.h"

#include <cstdio>

#include "../../app/Controller.h"
#include "../../io/Led.h"
#include "../../io/Settings.h"
#include "../Header.h"
#include "../PageStack.h"

SettingsPage::SettingsPage(AppContext &ctx) : _ctx(ctx) {
  _ledEnabled = Settings::ledEnabled();
  _airplaneMode = Settings::airplaneMode();
  _list.setSource(
      [&]() { return 3; },
      [&](int i, char *buf, int n) {
        if (i == 0)
          snprintf(buf, n, "Airplane: %s", _airplaneMode ? "ON " : "OFF");
        else if (i == 1)
          snprintf(buf, n, "LED:      %s", _ledEnabled ? "ON " : "OFF");
        else
          snprintf(buf, n, "< Back");
      });
}

void SettingsPage::render(Canvas &c, int yTop) { _list.render(c, yTop + 2); }

void SettingsPage::onShortPress(PageStack &nav) {
  _list.next();
  nav.requestRender();
}

void SettingsPage::onLongPress(PageStack &nav) {
  switch (_list.selected()) {
  case 0:
    _airplaneMode = !_airplaneMode;
    Settings::setAirplaneMode(_airplaneMode);
    _ctx.ctrl.setAirplaneMode(_airplaneMode);
    headerSetAirplaneMode(_airplaneMode);
    nav.requestRender();
    break;
  case 1:
    _ledEnabled = !_ledEnabled;
    Settings::setLedEnabled(_ledEnabled);
    _ctx.led.setEnabled(_ledEnabled);
    nav.requestRender();
    break;
  default:
    nav.leave();
    break;
  }
}
