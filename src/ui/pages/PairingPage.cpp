#include "PairingPage.h"

#include <cstdio>

#include "../PageStack.h"

void PairingPage::render(Canvas &c, int yTop) {
  // Labels at scale 1 so they fit the 122 px wide portrait panel.
  c.setTextSize(1);
  c.setCursor(4, yTop + 4);
  c.print("Enter on host:");

  char buf[12];
  snprintf(buf, sizeof(buf), "%06lu", (unsigned long)_pk);

  // Scale 3: 5x7 base becomes 15x21 = ~108 px for 6 digits, fits 122 px wide.
  c.setTextSize(3);
  int tw = c.textWidth(buf);
  c.setCursor((c.width() - tw) / 2, yTop + 18);
  c.print(buf);

  c.setTextSize(1);
  c.setCursor(4, c.height() - c.lineHeight() - 2);
  c.print("Press to dismiss");
  c.setTextSize(0);
}

void PairingPage::onShortPress(PageStack &nav) { nav.popModal(); }
void PairingPage::onLongPress(PageStack &nav) { nav.popModal(); }
