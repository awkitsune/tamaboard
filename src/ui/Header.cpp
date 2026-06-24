#include "Header.h"

#include <cstdio>

#include "../io/Battery.h"
#include "../io/Clock.h"
#include "Icons.h"

static bool s_locked = false;
static bool s_airplaneMode = false;
static const char *s_customText = nullptr;

void headerSetLocked(bool v) { s_locked = v; }
void headerSetAirplaneMode(bool v) { s_airplaneMode = v; }
void headerSetCustomText(const char *txt) { s_customText = txt; }

int drawHeader(Canvas &c, const char *title, bool entered) {
  c.setTextSize(1);
  const int H = 10;

  if (entered) {
    // Filled black bar: inversion signals "inside a page" at a glance.
    c.fillRect(0, 0, c.width(), H, true);
    c.setInverted(true);
  } else {
    c.setInverted(false);
  }

  c.setCursor(2, 1);
  c.print(title);

  char hhmm[6];
  Clock::format(hhmm, sizeof(hhmm));
  if (s_customText) {
    // Show custom text | clock, centered
    char buf[32];
    snprintf(buf, sizeof(buf), "%s | %s", s_customText, hhmm);
    c.setCursor(c.width() / 2 - c.textWidth(buf) / 2, 1);
    c.print(buf);
  } else {
    c.setCursor(c.width() / 2 - c.textWidth(hhmm) / 2, 1);
    c.print(hhmm);
  }

  char bat[6];
  snprintf(bat, sizeof(bat), "%u%%", Battery::percent());
  int batW = c.textWidth(bat);
  c.setCursor(c.width() - batW - 1, 1);
  c.print(bat);
  int iconX = c.width() - batW - 3;
  if (s_locked) {
    iconX -= ICON_LOCK_W;
    c.drawBitmap(iconX, 1, ICON_LOCK, ICON_LOCK_W, ICON_LOCK_H);
    iconX -= 2;
  }
  if (s_airplaneMode) {
    iconX -= ICON_AIRPLANE_W;
    c.drawBitmap(iconX, 1, ICON_AIRPLANE, ICON_AIRPLANE_W, ICON_AIRPLANE_H);
  }

  c.setInverted(false);
  // Separator only in carousel mode; the black-to-white edge is its own
  // separator when the header is filled.
  if (!entered)
    c.hline(0, H - 1, c.width());

  c.setTextSize(0);
  return H;
}
