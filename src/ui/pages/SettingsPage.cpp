#include "SettingsPage.h"
#include "../PageStack.h"
#include "../Header.h"
#include "../../app/Controller.h"
#include <cstdio>

SettingsPage::SettingsPage(AppContext& ctx) : _ctx(ctx) {
    _list.setSource(
        [&]() { return 2; },
        [&](int i, char* buf, int n) {
            if (i == 0) snprintf(buf, n, "Airplane: %s", _airplaneMode ? "ON " : "OFF");
            else        snprintf(buf, n, "< Back");
        }
    );
}

void SettingsPage::render(Canvas& c, int yTop) {
    _list.render(c, yTop + 2);
}

void SettingsPage::onShortPress(PageStack& nav) {
    _list.next();
    nav.requestRender();
}

void SettingsPage::onLongPress(PageStack& nav) {
    if (_list.selected() == 0) {
        _airplaneMode = !_airplaneMode;
        _ctx.ctrl.setAirplaneMode(_airplaneMode);
        headerSetAirplaneMode(_airplaneMode);
        nav.requestRender();
    } else {
        nav.leave();
    }
}
