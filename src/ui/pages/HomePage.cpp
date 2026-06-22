#include "HomePage.h"
#include "../PageStack.h"
#include "../../pet/PetFSM.h"
#include "../../io/EventLog.h"
#include <cstdio>

// Single-char label + thin bar, drawn at scale 1 (8 px text height).
// Bar is 4 px tall and vertically centred within the 8 px row.
static void drawStatRow(Canvas &c, int x, int y, int barX, int barW,
                        uint8_t pct, const char *label)
{
    c.setCursor(x, y);
    c.print(label);

    const int barH = 4;
    int by = y + (8 - barH) / 2; // vertically centre 4 px bar in 8 px text row
    c.fillRect(barX, by, barW, barH, true);
    c.fillRect(barX + 1, by + 1, barW - 2, barH - 2, false);
    int fill = (barW - 2) * pct / 100;
    if (fill > 0)
        c.fillRect(barX + 1, by + 1, fill, barH - 2, true);
}

// Returns a short ASCII face that fits within ~9 chars at 2x scale on the
// 122-px wide portrait panel.
static const char *faceFor(State s)
{
    switch (s)
    {
    case State::SLEEP:
        return "(-.-) zZz";
    case State::EAT:
        return "(=^.^=) *";
    case State::PLAY:
        return "(=^o^=) !";
    case State::KEYBOARD:
        return "(=^_^=) b";
    default:
        return "(=^.^=)";
    }
}

void HomePage::render(Canvas &c, int yTop)
{
    const Pet &p = _ctx.fsm.pet();

    // Lockscreen: sleep state shows only the face, centered, no stats or log.
    if (_ctx.fsm.state() == State::SLEEP)
    {
        const char *face = "(-.-) zZz";
        int fw = c.textWidth(face);
        c.setCursor((c.width() - fw) / 2, c.height() / 2 - 8);
        c.print(face);
        return;
    }

    // ---- Sprite zone: 48 px reserved for future pixel-art emoticon ----
    // The text face is centred inside this box so swapping in a sprite is a
    // one-liner: replace the c.print() block with a bitmap blit.
    static constexpr int SPRITE_H = 48;
    int spriteTop = yTop + 4;
    const char *face = faceFor(_ctx.fsm.state());
    int fw = c.textWidth(face); // 2x-scale text width
    int fh = 16;                // 2x-scale character height (8 * UI_SCALE)
    c.setCursor((c.width() - fw) / 2, spriteTop + (SPRITE_H - fh) / 2);
    c.print(face);

    // ---- Three compact stat rows, scale 1 ----
    // Single-char labels keep bx to 11 px, giving bars ~109 px width.
    c.setTextSize(1);
    const int rowH = 9; // 8 px text + 1 px gap
    int sx = 2;
    int bx = sx + 6 * 6 + 3; // 1-char label (6 px) + 3 px gap
    int bw = c.width() - bx - 2;
    int sy = spriteTop + SPRITE_H + 3;
    drawStatRow(c, sx, sy, bx, bw, p.hunger, "Hunger");
    drawStatRow(c, sx, sy + rowH, bx, bw, p.mood, "Mood");
    drawStatRow(c, sx, sy + rowH * 2, bx, bw, p.energy, "Energy");

    // ---- Activity log pinned to the bottom, scale 1 ----
    const int n = EventLog::count();
    if (n > 0)
    {
        int logRowH = c.lineHeight() / 2 + 1;
        int logY = c.height() - n * logRowH;
        c.hline(0, logY - 2, c.width());

        char hhmm[6], msg[EventLog::MSG_LEN];
        for (int i = 0; i < n; i++)
        {
            EventLog::get(i, hhmm, msg);
            c.setCursor(2, logY);
            c.print(hhmm);
            c.setCursor(38, logY);
            c.print(msg);
            logY += logRowH;
        }
    }
    c.setTextSize(0); // back to default for next frame
}

void HomePage::onShortPress(PageStack &nav) { nav.leave(); }
void HomePage::onLongPress(PageStack &nav) { nav.leave(); }
bool HomePage::interceptsRootLongPress(PageStack &nav) { nav.pushModal(&_care); return true; }
