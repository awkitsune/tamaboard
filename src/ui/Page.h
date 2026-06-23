#pragma once
#include "Canvas.h"

class PageStack;

// A page renders its own body (below the header). Navigation contract:
//   - Short press: page-defined (typically advances selection in a list).
//   - Long press : page-defined (typically selects/enters; if at top-level,
//                  PageStack rotates to the next page in the carousel).
struct Page {
    virtual ~Page() = default;
    virtual const char* title() = 0;
    virtual void render(Canvas& c, int yTop) = 0;
    virtual void onShortPress(PageStack& nav) = 0;
    virtual void onLongPress(PageStack& nav)  = 0;
    virtual void onEnter()  {}
    virtual void onLeave()  {}
    // Called by PageStack when this page is the current root and the user
    // long-presses. Return true to suppress the default "enter page" behavior.
    virtual bool interceptsRootLongPress(PageStack&) { return false; }

    // How often (ms) PageStack should auto-refresh this page when idle.
    // 0 = only refresh on explicit requestRender() calls.
    virtual uint32_t autoRefreshMs() const { return 0; }
};
