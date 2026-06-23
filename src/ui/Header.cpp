#include "Header.h"
#include "Icons.h"
#include "../io/Clock.h"
#include "../io/Battery.h"

static bool s_locked = false;
static bool s_airplaneMode = false;
static bool s_sleeping;

void headerSetLocked(bool v)      { s_locked = v; }
void headerSetAirplaneMode(bool v) { s_airplaneMode = v; }
void headerSetSleeping(bool v)     { s_sleeping = v; }

static void drawBattery(Canvas &c, int x, int y, uint8_t pct)
{
    // 14x6 battery: 1px outline, 2px nub, 4 fill cells.
    c.fillRect(x, y, 12, 6, true);
    c.fillRect(x + 1, y + 1, 10, 4, false);
    c.fillRect(x + 12, y + 2, 2, 2, true);
    int cells = (pct + 24) / 25; // 0..4
    for (int i = 0; i < cells; i++)
        c.fillRect(x + 1 + i * 3, y + 1, 2, 4, true);
}

int drawHeader(Canvas &c, const char *title, bool entered)
{
    c.setTextSize(1);
    const int H = 10;

    if (entered)
    {
        // Filled black bar: inversion signals "inside a page" at a glance.
        c.fillRect(0, 0, c.width(), H, true);
        c.setInverted(true);
    }
    else
    {
        c.setInverted(false);
    }

    c.setCursor(2, 1);
    c.print(title);

    if (s_sleeping)
    {
        const char *txt = "SLEEP";
        c.setCursor(c.width() / 2 - c.textWidth(txt) / 2, 1);
        c.print(txt);
    }
    else
    {
        char hhmm[6];
        Clock::format(hhmm, sizeof(hhmm));
        c.setCursor(c.width() / 2 - c.textWidth(hhmm) / 2, 1);
        c.print(hhmm);
    }

    drawBattery(c, c.width() - 16, 1, Battery::percent());
    int iconX = c.width() - 16 - 2;
    if (s_locked)
    {
        iconX -= ICON_LOCK_W;
        c.drawBitmap(iconX, 1, ICON_LOCK, ICON_LOCK_W, ICON_LOCK_H);
        iconX -= 2;
    }
    if (s_airplaneMode)
    {
        iconX -= ICON_AIRPLANE_W;
        c.drawBitmap(iconX, 1, ICON_AIRPLANE, ICON_AIRPLANE_W, ICON_AIRPLANE_H);
    }

    c.setInverted(false);
    // Separator only in carousel mode; the black-to-white edge is its own separator
    // when the header is filled.
    if (!entered)
        c.hline(0, H - 1, c.width());

    c.setTextSize(0);
    return H;
}
