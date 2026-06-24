#include "PageStack.h"

#include <Arduino.h>

#include "Header.h"

PageStack::PageStack(Display &display) : _display(display) {
  portMUX_INITIALIZE(&_navMux);
  _renderSem = xSemaphoreCreateBinary();
}

Page *PageStack::currentPage() const {
  if (_modal)
    return _modal;
  if (_roots.empty())
    return nullptr;
  return _roots[_rootIdx];
}

// ---- Navigation (Core 0) ------------------------------------------------

void PageStack::onShortPress() {
  if (_suppressNextInput) {
    _suppressNextInput = false;
    return;
  }

  bool wasPaused = _screenPaused;
  activityBump();
  if (wasPaused)
    return; // first press just unpauses, does not navigate

  if (_modal || _entered) {
    if (auto *p = currentPage())
      p->onShortPress(*this);
  } else if (!_roots.empty()) {
    portENTER_CRITICAL(&_navMux);
    _rootIdx = (_rootIdx + 1) % (int)_roots.size();
    portEXIT_CRITICAL(&_navMux);
    requestRender();
  }
}

void PageStack::onLongPress() {
  if (_suppressNextInput) {
    _suppressNextInput = false;
    return;
  }

  bool wasPaused = _screenPaused;
  activityBump();
  if (wasPaused)
    return;

  if (_modal || _entered) {
    if (auto *p = currentPage())
      p->onLongPress(*this);
  } else if (!_roots.empty()) {
    // Capture page pointer before releasing the lock so we can call callbacks
    // outside of it (callbacks may call nav methods that re-acquire _navMux).
    Page *p;
    portENTER_CRITICAL(&_navMux);
    p = currentPage();
    portEXIT_CRITICAL(&_navMux);

    if (p && p->interceptsRootLongPress(*this))
      return;

    portENTER_CRITICAL(&_navMux);
    _entered = true;
    portEXIT_CRITICAL(&_navMux);
    if (p)
      p->onEnter();
    requestRender();
  }
}

void PageStack::leave() {
  if (_modal) {
    popModal();
    return;
  }
  if (_entered) {
    Page *p;
    portENTER_CRITICAL(&_navMux);
    p = currentPage();
    _entered = false;
    portEXIT_CRITICAL(&_navMux);
    if (p)
      p->onLeave();
    requestRender();
  }
}

void PageStack::goHome() {
  Page *exitModal = nullptr, *exitPage = nullptr;
  portENTER_CRITICAL(&_navMux);
  if (_modal) {
    exitModal = _modal;
  } else if (_entered && !_roots.empty()) {
    exitPage = _roots[_rootIdx];
  }
  _modal = nullptr;
  _entered = false;
  _rootIdx = 0;
  portEXIT_CRITICAL(&_navMux);
  if (exitModal)
    exitModal->onLeave();
  if (exitPage)
    exitPage->onLeave();
  requestRender();
}

void PageStack::pushModal(Page *p) {
  portENTER_CRITICAL(&_navMux);
  _modal = p;
  portEXIT_CRITICAL(&_navMux);
  if (p)
    p->onEnter();
  requestRender();
}

void PageStack::popModal() {
  Page *p;
  portENTER_CRITICAL(&_navMux);
  p = _modal;
  _modal = nullptr;
  portEXIT_CRITICAL(&_navMux);
  if (p)
    p->onLeave();
  requestRender();
}

// ---- Render signaling (Core 0) -------------------------------------------

void PageStack::requestRender() {
  _dirty.store(true, std::memory_order_release);
  if (!_screenPaused)
    xSemaphoreGive(_renderSem);
}

// ---- Render paths --------------------------------------------------------

// Blocking — runs as the body of uiTask on Core 1.
void PageStack::renderIfDirty() {
  xSemaphoreTake(_renderSem, portMAX_DELAY);
  if (!_dirty.load(std::memory_order_acquire))
    return; // spurious wake guard
  _dirty.store(false, std::memory_order_relaxed);

  // Snapshot nav state under the lock so we don't race with Core 0 mutations.
  Page *page;
  bool wasEntered;
  portENTER_CRITICAL(&_navMux);
  page = currentPage();
  wasEntered = entered();
  portEXIT_CRITICAL(&_navMux);

  if (!page)
    return;
  _display.draw([&](Canvas &c) {
    int yTop = drawHeader(c, page->title(), wasEntered);
    page->render(c, yTop + 1);
  });
  _lastRenderMs = millis();
}

// Non-blocking — called from Core 0 (button task) during sleep entry, while
// uiTask is suspended. Bypasses the semaphore entirely.
void PageStack::renderDirectly() {
  _dirty.store(false, std::memory_order_relaxed);
  Page *page = currentPage();
  bool wasEntered = entered();
  if (!page)
    return;
  _display.draw([&](Canvas &c) {
    int yTop = drawHeader(c, page->title(), wasEntered);
    page->render(c, yTop + 1);
  });
  _lastRenderMs = millis();
}

// ---- Auto-refresh (Core 0 loop) ------------------------------------------

void PageStack::checkAutoRefresh() {
  if (_screenPaused || _dirty.load(std::memory_order_relaxed))
    return;
  Page *p;
  portENTER_CRITICAL(&_navMux);
  p = currentPage();
  portEXIT_CRITICAL(&_navMux);
  if (!p)
    return;
  uint32_t pageMs = p->autoRefreshMs();
  uint32_t best;
  if (pageMs > 0 && (_globalRefreshMs == 0 || pageMs < _globalRefreshMs))
    best = pageMs;
  else
    best = _globalRefreshMs;
  if (best > 0 && millis() - _lastRenderMs >= best)
    requestRender();
}

// ---- Pause (Core 0) ------------------------------------------------------

void PageStack::setPauseTimeout(uint32_t ms) {
  _pauseTimeoutMs = ms;
  _lastActivityMs = millis();
}

void PageStack::setScreenPaused(bool v) {
  if (v == _screenPaused)
    return;
  if (v) {
    headerSetCustomText("PAUSED");
    _dirty.store(true, std::memory_order_release);
    _screenPaused = true;
    // Wake the UI task for one final "PAUSED" render; it will block again
    // once rendered because requestRender() guards on _screenPaused.
    xSemaphoreGive(_renderSem);
    Serial.println("[PageStack] screen paused");
  } else {
    _screenPaused = false;
    suppressNextInput(); // don't navigate on the wakeup press
    headerSetCustomText(nullptr);
    _display.markFullRefresh();
    _dirty.store(true, std::memory_order_release);
    xSemaphoreGive(_renderSem);
    Serial.println("[PageStack] screen unpaused");
  }
}

void PageStack::activityBump() {
  _lastActivityMs = millis();
  setScreenPaused(false); // no-op if already active
}
