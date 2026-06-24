#pragma once
#include "../../AppContext.h"
#include "../ListView.h"
#include "../Page.h"

// Modal submenu: context-sensitive actions for one BLE host slot.
// Caller sets the slot with prepare(i) before pushing via nav.pushModal().
class SlotActionPage : public Page {
public:
  explicit SlotActionPage(AppContext &ctx);

  void prepare(uint8_t slotIdx); // call before pushModal

  const char *title() override { return _title; }
  void render(Canvas &c, int yTop) override;
  void onShortPress(PageStack &nav) override;
  void onLongPress(PageStack &nav) override;
  void onEnter() override { _list.setSelected(0); }

private:
  int actionCount() const;
  void actionLabel(int i, char *buf, int n) const;
  void executeAction(int i, PageStack &nav);

  AppContext &_ctx;
  uint8_t _slotIdx = 0;
  char _title[16] = "SLOT";
  ListView _list;
};
