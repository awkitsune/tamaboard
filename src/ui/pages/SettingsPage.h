#pragma once
#include "../../AppContext.h"
#include "../ListView.h"
#include "../Page.h"

class SettingsPage : public Page {
public:
  explicit SettingsPage(AppContext &ctx);
  const char *title() override { return "SETTINGS"; }
  void render(Canvas &c, int yTop) override;
  void onShortPress(PageStack &nav) override;
  void onLongPress(PageStack &nav) override;

private:
  AppContext &_ctx;
  ListView _list;
  bool _airplaneMode = false;
  bool _ledEnabled = true;
};
