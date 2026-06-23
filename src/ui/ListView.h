#pragma once
#include <cstdint>
#include <functional>

#include "Canvas.h"

// Vertical list widget. Caller supplies items via `count()` + `itemText(i, buf,
// n)`; the selected item is drawn inverted (foreground/background swapped).
//
// Navigation: next() advances selection (wraps).
class ListView {
public:
  using CountFn = std::function<int()>;
  using ItemFn = std::function<void(int i, char *buf, int bufN)>;

  ListView() = default;
  ListView(CountFn count, ItemFn item) : _count(count), _item(item) {}

  void setSource(CountFn count, ItemFn item) {
    _count = count;
    _item = item;
    _selected = 0;
  }
  void next() {
    int n = _count();
    if (n)
      _selected = (_selected + 1) % n;
  }
  int selected() const { return _selected; }
  void setSelected(int i) { _selected = i; }

  // Renders starting at y, returning the y after the last item.
  int render(Canvas &c, int yStart) const;

private:
  CountFn _count;
  ItemFn _item;
  int _selected = 0;
  mutable int _scrollOffset =
      0; // adjusted in render() to keep selected visible
};
