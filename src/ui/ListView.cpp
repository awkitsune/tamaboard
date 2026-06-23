#include "ListView.h"

int ListView::render(Canvas &c, int yStart) const {
  const int rowH = c.lineHeight() + 2;
  const int innerY = 2;
  const int n = _count ? _count() : 0;
  const int vis = (c.height() - yStart) / rowH; // items that fit on screen

  // Keep scroll offset clamped so the selected row is always visible.
  if (_selected < _scrollOffset)
    _scrollOffset = _selected;
  if (_selected >= _scrollOffset + vis)
    _scrollOffset = _selected - vis + 1;
  if (_scrollOffset < 0)
    _scrollOffset = 0;

  int y = yStart;
  char buf[40];

  for (int i = _scrollOffset; i < n && y + rowH <= c.height(); i++) {
    if (_item)
      _item(i, buf, sizeof(buf));
    else
      buf[0] = '\0';

    bool sel = (i == _selected);
    if (sel)
      c.fillRect(0, y, c.width(), rowH, true);
    c.setInverted(sel);
    c.setCursor(4, y + innerY);
    c.print(buf);
    y += rowH;
  }
  c.setInverted(false);
  return y;
}
