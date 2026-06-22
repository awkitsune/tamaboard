#include "CarePage.h"
#include "../PageStack.h"
#include "../../app/Controller.h"
#include <cstring>

static const char* ACTIONS[] = { "Feed", "Play", "Sleep" };
static const State STATES[]  = { State::EAT, State::PLAY, State::SLEEP };
static constexpr int N_ACTIONS = sizeof(ACTIONS) / sizeof(ACTIONS[0]);

CarePage::CarePage(AppContext& ctx) : _ctx(ctx) {
    _list.setSource(
        []() { return N_ACTIONS; },
        [](int i, char* buf, int n) { strncpy(buf, ACTIONS[i], n); buf[n - 1] = '\0'; }
    );
}

void CarePage::onEnter() { _list.setSelected(0); }

void CarePage::render(Canvas& c, int yTop) {
    _list.render(c, yTop + 2);
}

void CarePage::onShortPress(PageStack& nav) {
    _list.next();
    nav.requestRender();
}

void CarePage::onLongPress(PageStack& nav) {
    _ctx.ctrl.transitionState(STATES[_list.selected()]);
    nav.popModal();
}
