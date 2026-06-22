#pragma once
#include "../Page.h"
#include "../../AppContext.h"
#include "CarePage.h"

class HomePage : public Page {
public:
    explicit HomePage(AppContext& ctx) : _ctx(ctx), _care(ctx) {}
    const char* title() override { return "HOME"; }
    void render(Canvas& c, int yTop) override;
    void onShortPress(PageStack& nav) override;
    void onLongPress(PageStack& nav) override;
    bool interceptsRootLongPress(PageStack& nav) override;

private:
    AppContext& _ctx;
    CarePage    _care;
};
