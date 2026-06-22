#include "KeyboardPage.h"
#include "../PageStack.h"
#include "../../app/Controller.h"
#include "../../hid/BleHid.h"
#include "../../io/DeviceSlots.h"
#include "../../pet/PetFSM.h"
#include <cstdio>

KeyboardPage::KeyboardPage(AppContext& ctx) : _ctx(ctx), _slotActions(ctx) {
    _list.setSource(
        [&]() { return (int)SLOT_COUNT + 2; },
        [&](int i, char* buf, int n) {
            if (i == (int)SLOT_COUNT) {
                snprintf(buf, n, "BLE: %s", _ctx.fsm.inKeyboardMode() ? "ON " : "OFF");
                return;
            }
            if (i == (int)SLOT_COUNT + 1) {
                snprintf(buf, n, "< Back");
                return;
            }
            const auto& sl = _ctx.slots[i];
            const char* tag = sl.occupied
                ? (_ctx.slots.active() == (uint8_t)i && _ctx.ble.status() == BleHidStatus::CONNECTED
                    ? "*"   // active + connected
                    : "+")  // bonded
                : "-";      // empty
            snprintf(buf, n, "%s %s", tag, sl.name);
        }
    );
}

void KeyboardPage::render(Canvas& c, int yTop) {
    int y = _list.render(c, yTop + 2);
    // Footer: live BLE status at scale 1 so it fits the 122-px-wide portrait panel.
    static auto bleLabel = [](BleHidStatus s) -> const char* {
        switch (s) {
            case BleHidStatus::IDLE:        return "BLE off";
            case BleHidStatus::ADVERTISING: return "Searching...";
            case BleHidStatus::PAIRING:     return "Check host for PIN";
            case BleHidStatus::CONNECTED:   return "Connected";
            case BleHidStatus::FAILED:      return "Pairing failed";
        }
        return "";
    };
    c.setTextSize(1);
    c.setCursor(2, c.height() - c.lineHeight() - 1);
    c.print(bleLabel(_ctx.ble.status()));
    c.setTextSize(0);
    (void)y;
}

void KeyboardPage::onShortPress(PageStack& nav) {
    _list.next();
    nav.requestRender();
}

void KeyboardPage::onLongPress(PageStack& nav) {
    int sel = _list.selected();
    if (sel == (int)SLOT_COUNT) {
        // Toggle BLE keyboard on/off without leaving the page.
        if (_ctx.fsm.inKeyboardMode())
            _ctx.ctrl.transitionState(State::IDLE);
        else
            _ctx.ctrl.transitionState(State::KEYBOARD);
        nav.requestRender();
        return;
    }
    if (sel == (int)SLOT_COUNT + 1) {
        // Back: leave without changing BLE state so keyboard keeps running.
        nav.leave();
        return;
    }
    _slotActions.prepare((uint8_t)sel);
    nav.pushModal(&_slotActions);
}
