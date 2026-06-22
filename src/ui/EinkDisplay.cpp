#include "EinkDisplay.h"
#include <heltec-eink-modules.h>

// Wireless Paper v1.2 ships the E0213A367 panel (NOT the older LCMEN2R13EFC1
// that v1.1 used — they have different init sequences, picking wrong panel
// hangs the library forever on the BUSY pin). EInkDisplay_WirelessPaperV1_2
// is just the friendly alias. WIRELESS_PAPER define activates the no-arg
// constructor and the GPIO45 Vext power control built into the library.
static EInkDisplay_WirelessPaperV1_2 g_display;

// Default UI scale: Adafruit-GFX built-in 5x7 font, scaled 2x → 12x16 cells.
// ~20 chars per line, ~6 lines of body content after the header. Pages that
// want smaller text (log rows, footer hints) call setTextSize(1) explicitly,
// then setTextSize(0) to return to default.
static constexpr int UI_SCALE = 2;
static constexpr int LINE_HEIGHT = 8 * UI_SCALE;

// Anti-ghost cap: after this many partial refreshes on the same page, force a
// full refresh to clear accumulated artifacts.
static constexpr int MAX_PARTIALS = 100;

class EinkCanvas : public Canvas
{
    bool _inverted = false;
public:
    int width() const override { return g_display.width(); }
    int height() const override { return g_display.height(); }

    void clear() override { g_display.fillScreen(WHITE); }
    void fillRect(int x, int y, int w, int h, bool on) override
    {
        bool actual = _inverted ? !on : on;
        g_display.fillRect(x, y, w, h, actual ? BLACK : WHITE);
    }
    void hline(int x, int y, int w) override
    {
        g_display.drawLine(x, y, x + w - 1, y, _inverted ? WHITE : BLACK);
    }
    void drawBitmap(int x, int y, const uint8_t *data, int w, int h) override
    {
        g_display.drawBitmap(x, y, data, w, h, _inverted ? WHITE : BLACK);
    }

    void setCursor(int x, int y) override { g_display.setCursor(x, y); }
    void setInverted(bool inv) override
    {
        _inverted = inv;
        g_display.setTextColor(inv ? WHITE : BLACK);
    }
    void setTextSize(uint8_t s) override { g_display.setTextSize(s == 0 ? UI_SCALE : s); }
    void print(const char *s) override { g_display.print(s); }
    void printf(const char *fmt, ...) override
    {
        char buf[128];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        g_display.print(buf);
    }
    int textWidth(const char *s) override
    {
        int16_t x1, y1;
        uint16_t w, h;
        g_display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
        return w;
    }
    int lineHeight() override { return LINE_HEIGHT; }
};

void EinkDisplay::begin()
{
    g_display.begin();
    // Board is mounted with the ribbon cable at the bottom, so the long axis
    // of the panel is vertical. Rotate 90° to portrait — width()/height() now
    // report 122/250 instead of the native 250/122. If the UI ends up
    // upside-down, switch to setRotation(3).
    g_display.setRotation(3);
    g_display.setTextWrap(false);
}

void EinkDisplay::markFullRefresh()
{
    _fullPending = true;
}

void EinkDisplay::draw(std::function<void(Canvas &)> fn)
{
    g_display.setWindow(0, 0, g_display.width(), g_display.height());

    const bool doFull = _fullPending || _partialsSinceFull >= MAX_PARTIALS;
    if (doFull)
    {
        g_display.fastmodeOff();
    }

    EinkCanvas canvas;
    DRAW(g_display)
    {
        canvas.clear();
        g_display.setTextSize(UI_SCALE); // reset between frames
        fn(canvas);
    }

    if (doFull)
    {
        g_display.fastmodeOn();
        _fullPending = false;
        _partialsSinceFull = 0;
    }
    else
    {
        _partialsSinceFull++;
    }
}

void EinkDisplay::powerOff()
{
    // heltec-eink-modules deasserts Vext (GPIO45) between operations on its own.
}
