#pragma once
#include "../Page.h"
#include "../ListView.h"
#include "../../AppContext.h"

class CarePage : public Page {
public:
    explicit CarePage(AppContext& ctx);
    const char* title() override { return "CARE"; }
    void render(Canvas& c, int yTop) override;
    void onEnter() override;
    void onShortPress(PageStack& nav) override;
    void onLongPress(PageStack& nav)  override;

private:
    AppContext& _ctx;
    ListView    _list;
};
