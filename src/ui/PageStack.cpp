#include "PageStack.h"

#include <Arduino.h>

#include "Header.h"

Page *PageStack::currentPage() const {
  if (_modal)
    return _modal;
  if (_roots.empty())
    return nullptr;
  return _roots[_rootIdx];
}

void PageStack::onShortPress() {
  if (_suppressNextInput) {
    _suppressNextInput = false;
    return; // silently drop this input
  }

  bool wasPaused = _screenPaused;
  activityBump();
  if (wasPaused)
    return; // first press just unpauses, does not navigate

  if (_modal || _entered) {
    if (auto *p = currentPage())
      p->onShortPress(*this);
  } else if (!_roots.empty()) {
    _rootIdx = (_rootIdx + 1) % _roots.size();
    _dirty = true;
  }
}

void PageStack::onLongPress() {
  if (_suppressNextInput) {
    _suppressNextInput = false;
    return; // silently drop this input
  }

  bool wasPaused = _screenPaused;
  activityBump();
  if (wasPaused)
    return; // first press just unpauses, does not navigate

  if (_modal || _entered) {
    if (auto *p = currentPage())
      p->onLongPress(*this);
  } else if (!_roots.empty()) {
    if (auto *p = currentPage())
      if (p->interceptsRootLongPress(*this))
        return;
    _entered = true;
    if (auto *p = currentPage())
      p->onEnter();
    _dirty = true;
  }
}

void PageStack::leave() {
  if (_modal) {
    popModal();
    return;
  }
  if (_entered) {
    if (auto *p = currentPage())
      p->onLeave();
    _entered = false;
    _dirty = true;
  }
}

void PageStack::goHome() {
  if (_modal) {
    _modal->onLeave();
    _modal = nullptr;
  }
  if (_entered) {
    if (auto *p = currentPage())
      p->onLeave();
    _entered = false;
  }
  _rootIdx = 0;
  _dirty = true;
}

void PageStack::pushModal(Page *p) {
  _modal = p;
  if (p)
    p->onEnter();
  _dirty = true;
}

void PageStack::popModal() {
  if (_modal) {
    _modal->onLeave();
    _modal = nullptr;
    _dirty = true;
  }
}

void PageStack::renderIfDirty() {
  if (_screenPaused)
    return; // preserve _dirty so we render immediately on unpause
  if (!_dirty)
    return;
  _dirty = false;

  Page *page = currentPage();
  if (!page)
    return;

  _display.draw([&](Canvas &c) {
    int yTop = drawHeader(c, page->title(), entered());
    page->render(c, yTop + 1);
  });
  _lastRenderMs = millis();
}

void PageStack::checkAutoRefresh() {
  if (_screenPaused || _dirty)
    return;
  Page *p = currentPage();
  if (!p)
    return;
  uint32_t pageMs = p->autoRefreshMs();
  // Pick whichever interval fires first; 0 means "no constraint from that
  // source".
  uint32_t best;
  if (pageMs > 0 && (_globalRefreshMs == 0 || pageMs < _globalRefreshMs))
    best = pageMs;
  else
    best = _globalRefreshMs;
  if (best > 0 && millis() - _lastRenderMs >= best)
    requestRender();
}

void PageStack::setPauseTimeout(uint32_t ms) {
  _pauseTimeoutMs = ms;
  _lastActivityMs = millis(); // start the idle clock from now
}

void PageStack::setScreenPaused(bool v) {
  if (v == _screenPaused)
    return;
  if (v) {
    headerSetCustomText("PAUSED");
    // Render "PAUSED" header while _screenPaused is still false so
    // renderIfDirty() doesn't bail, then lock the display.
    _dirty = true;
    renderIfDirty();
    _screenPaused = true;
    Serial.println("[PageStack] screen paused");
  } else {
    _screenPaused = false;
    suppressNextInput(); // don't navigate on the wakeup press
    headerSetCustomText(nullptr);
    // Waking up — e-ink may have ghosted; force a clean full refresh.
    _display.markFullRefresh();
    requestRender();
    Serial.println("[PageStack] screen unpaused");
  }
}

void PageStack::activityBump() {
  _lastActivityMs = millis();
  setScreenPaused(false); // no-op if already active
}
