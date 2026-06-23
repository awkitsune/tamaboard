#pragma once
#include <functional>

#include "Canvas.h"

// A Display owns the panel hardware. draw() hands the caller a Canvas inside
// the driver's frame loop (paged for e-ink, double-buffered for OLED, etc.).
class Display {
public:
  virtual ~Display() = default;
  virtual void begin() = 0;
  virtual void draw(std::function<void(Canvas &)> fn) = 0;
  virtual void powerOff() = 0;
  // Request that the next draw use a full refresh (rather than partial / fast
  // mode). Backends without a partial-refresh mode can leave this as a no-op.
  virtual void markFullRefresh() {}
};
