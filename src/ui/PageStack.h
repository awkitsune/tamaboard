#pragma once
#include <atomic>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "Display.h"
#include "Page.h"

// Carousel of root pages. Navigation contract:
//   - At root carousel: short press → next page, long press → enter the page.
//   - Inside an entered page: short / long forwarded to the page.
//   - A page can call nav.leave() to return to the root carousel.
//   - A page can call nav.pushModal(p) to overlay a modal (e.g. pairing PIN).
//
// Thread model (Step 7):
//   Core 0 — button task and loop task mutate nav state and call requestRender().
//   Core 1 — uiTask runs renderIfDirty() in a tight loop, blocking on _renderSem.
//   Nav state (_rootIdx, _entered, _modal) is snapshotted under _navMux before
//   each render. _dirty is std::atomic; _screenPaused/_suppressNextInput are volatile.
class PageStack {
public:
  explicit PageStack(Display &display);

  void addRoot(Page *p) { _roots.push_back(p); }

  void onShortPress();
  void onLongPress();

  // Suppress the next button event entirely (useful for unpause / sleep wake).
  void suppressNextInput() { _suppressNextInput = true; }

  void leave();
  void goHome();
  void pushModal(Page *p);
  void popModal();

  // Signal the UI task to render on the next cycle.
  void requestRender();
  void markFullRefresh() { _display.markFullRefresh(); }

  // Blocking render — runs in the UI task on Core 1. Waits on _renderSem.
  void renderIfDirty();

  // Non-blocking render — called from Core 0 while the UI task is suspended
  // (sleep-entry path). Does not touch the semaphore.
  void renderDirectly();

  // Call from loop() — triggers a render if the current page's autoRefreshMs()
  // (or the global 30 s clock tick, whichever fires first) has elapsed.
  void checkAutoRefresh();

  // Screen-pause: display stops updating after pauseTimeoutMs of inactivity.
  // The first button press after a pause unpauses without navigating.
  void setPauseTimeout(uint32_t ms); // 0 = disabled; also resets activity clock
  void setScreenPaused(bool v);      // explicit pause/unpause
  void activityBump(); // call on any user interaction to reset the idle clock
  bool isScreenPaused() const { return _screenPaused; }
  uint32_t lastActivityMs() const { return _lastActivityMs; }
  uint32_t pauseTimeoutMs() const { return _pauseTimeoutMs; }

  Page *currentPage() const;
  bool entered() const { return _entered || _modal; }

private:
  Display &_display;
  std::vector<Page *> _roots;

  // Nav state — written Core 0, snapshot by Core 1 under _navMux before render.
  portMUX_TYPE _navMux;
  int _rootIdx  = 0;
  bool _entered = false;
  Page *_modal  = nullptr;

  // Render signaling
  std::atomic<bool> _dirty{true};
  SemaphoreHandle_t _renderSem = nullptr;

  uint32_t _globalRefreshMs = 30000; // clock header ticks for all pages

  // Written by Core 1 after render, read by Core 0 in checkAutoRefresh().
  // 32-bit aligned — a one-cycle-stale read in checkAutoRefresh() is acceptable.
  volatile uint32_t _lastRenderMs = 0;

  // Written Core 0, read both cores — volatile ensures the compiler doesn't
  // cache these in a register across the task boundary.
  volatile bool _screenPaused      = false;
  volatile bool _suppressNextInput = false;

  uint32_t _lastActivityMs = 0;
  uint32_t _pauseTimeoutMs = 0; // 0 = disabled
};
