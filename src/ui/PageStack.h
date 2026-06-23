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

    // Suppress the next button event entirely (useful for state transitions like
    // unpause or sleep wake where we don't want the wakeup press to navigate).
    void suppressNextInput() { _suppressNextInput = true; }

    void leave();
    void goHome();
    void pushModal(Page* p);
    void popModal();

    void requestRender()    { _dirty = true; }
    void markFullRefresh()  { _display.markFullRefresh(); }
    void renderIfDirty();

    // Call from loop() — triggers a render if the current page's autoRefreshMs()
    // (or the global 30 s clock tick, whichever fires first) has elapsed.
    void checkAutoRefresh();

    // Screen-pause: display stops updating after pauseTimeoutMs of inactivity.
    // The first button press after a pause unpauses without navigating.
    void     setPauseTimeout(uint32_t ms);   // 0 = disabled; also resets activity clock
    void     setScreenPaused(bool v);        // explicit pause/unpause
    void     activityBump();                 // call on any user interaction to reset the idle clock
    bool     isScreenPaused()  const { return _screenPaused; }
    uint32_t lastActivityMs()  const { return _lastActivityMs; }
    uint32_t pauseTimeoutMs()  const { return _pauseTimeoutMs; }

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
    bool                _screenPaused      = false;
    bool                _suppressNextInput = false;
    uint32_t            _lastActivityMs    = 0;
    uint32_t            _pauseTimeoutMs    = 0;     // 0 = disabled
};
