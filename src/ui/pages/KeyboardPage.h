#pragma once
#include "../../AppContext.h"
#include "../ListView.h"
#include "../Page.h"
#include "SlotActionPage.h"

class KeyboardPage : public Page {
public:
  explicit KeyboardPage(AppContext &ctx);
  const char *title() override { return "KEYBOARD"; }
  void render(Canvas &c, int yTop) override;
  void onShortPress(PageStack &nav) override;
  void onLongPress(PageStack &nav) override;

private:
  AppContext &_ctx;
  ListView _list;
  SlotActionPage _slotActions; // modal pushed on slot long-press
};
