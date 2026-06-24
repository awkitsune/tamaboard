#include "SlotActionPage.h"

#include <cstdio>
#include <cstring>

#include "../../app/Controller.h"
#include "../../hid/BleHid.h"
#include "../../io/DeviceSlots.h"
#include "../PageStack.h"

// Action labels depend on the slot's current state.
// Empty  → Pair / Cancel
// Bonded, not connected → Connect / Forget / Cancel
// Connected             → Forget  / Cancel
static const char *const LABELS_EMPTY[] = {"Pair", "Cancel"};
static const char *const LABELS_INACTIVE[] = {"Connect", "Forget", "Cancel"};
static const char *const LABELS_ACTIVE[] = {"Forget", "Cancel"};

struct ActionSet {
  const char *const *labels;
  int count;
};

static ActionSet pick(const AppContext &ctx, uint8_t idx) {
  const auto &sl = ctx.slots[idx];
  if (!sl.occupied)
    return {LABELS_EMPTY, 2};
  bool connected =
      ctx.slots.active() == idx && ctx.ble.status() == BleHidStatus::CONNECTED;
  if (connected)
    return {LABELS_ACTIVE, 2};
  return {LABELS_INACTIVE, 3};
}

SlotActionPage::SlotActionPage(AppContext &ctx) : _ctx(ctx) {
  _list.setSource([&]() { return pick(_ctx, _slotIdx).count; },
                  [&](int i, char *buf, int n) {
                    ActionSet as = pick(_ctx, _slotIdx);
                    snprintf(buf, n, "%s", i < as.count ? as.labels[i] : "");
                  });
}

void SlotActionPage::prepare(uint8_t slotIdx) {
  _slotIdx = slotIdx;
  snprintf(_title, sizeof(_title), "SLOT %u", (unsigned)(slotIdx + 1));
  _list.setSelected(0);
}

void SlotActionPage::render(Canvas &c, int yTop) { _list.render(c, yTop + 2); }

void SlotActionPage::onShortPress(PageStack &nav) {
  _list.next();
  nav.requestRender();
}

void SlotActionPage::onLongPress(PageStack &nav) {
  ActionSet as = pick(_ctx, _slotIdx);
  int sel = _list.selected();
  if (sel >= as.count) {
    nav.popModal();
    return;
  }

  const char *label = as.labels[sel];
  using SA = Controller::SlotAction;

  if (strcmp(label, "Pair") == 0) {
    _ctx.ctrl.transitionState(State::KEYBOARD);
    _ctx.ctrl.slotAction(_slotIdx, SA::PAIR);
  } else if (strcmp(label, "Connect") == 0) {
    _ctx.ctrl.transitionState(State::KEYBOARD);
    _ctx.ctrl.slotAction(_slotIdx, SA::SELECT);
  } else if (strcmp(label, "Forget") == 0) {
    _ctx.ctrl.slotAction(_slotIdx, SA::FORGET);
  }
  // Cancel falls through — all paths pop the modal.
  nav.popModal();
}
