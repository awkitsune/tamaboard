#pragma once
#include "Page.h"
#include "Display.h"
#include <vector>

// Carousel of root pages. Navigation contract:
//   - At root carousel: short press → next page, long press → enter the page.
//   - Inside an entered page: short / long forwarded to the page.
//   - A page can call nav.leave() to return to the root carousel.
//   - A page can call nav.pushModal(p) to overlay a modal (e.g. pairing PIN).
class PageStack {
public:
    explicit PageStack(Display& display) : _display(display) {}

    void addRoot(Page* p) { _roots.push_back(p); }

    void onShortPress();
    void onLongPress();

    void leave();
    void goHome();
    void pushModal(Page* p);
    void popModal();

    void requestRender() { _dirty = true; }
    void renderIfDirty();

    // Call from loop() — triggers a render if the current page's autoRefreshMs()
    // (or the global 30 s clock tick, whichever fires first) has elapsed.
    void checkAutoRefresh();

    Page* currentPage() const;
    bool  entered() const { return _entered || _modal; }

private:
    Display&            _display;
    std::vector<Page*>  _roots;
    int                 _rootIdx = 0;
    bool                _entered = false;
    Page*               _modal   = nullptr;
    bool                _dirty            = true;
    uint32_t            _globalRefreshMs  = 30000; // clock header ticks for all pages
    uint32_t            _lastRenderMs     = 0;
};
