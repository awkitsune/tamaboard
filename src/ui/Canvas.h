#pragma once
#include <cstdarg>
#include <cstdint>

// Minimal mono drawing surface. on=true means foreground (e.g. black ink),
// false means background (e.g. white paper). Drivers wrap their native API.
class Canvas {
public:
  virtual ~Canvas() = default;

  virtual int width() const = 0;
  virtual int height() const = 0;

  virtual void clear() = 0;
  virtual void fillRect(int x, int y, int w, int h, bool on) = 0;
  virtual void hline(int x, int y, int w) = 0;
  virtual void drawBitmap(int x, int y, const uint8_t *data, int w, int h) = 0;

  virtual void setCursor(int x, int y) = 0;
  virtual void setInverted(bool inv) = 0;
  virtual void setTextSize(uint8_t s) = 0;
  virtual void print(const char *s) = 0;
  virtual void printf(const char *fmt, ...)
      __attribute__((format(printf, 2, 3))) = 0;
  virtual int textWidth(const char *s) = 0;
  virtual int lineHeight() = 0;
};
