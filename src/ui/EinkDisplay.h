#pragma once
#include "Display.h"

class EinkDisplay : public Display {
public:
  void begin() override;
  void draw(std::function<void(Canvas &)> fn) override;
  void powerOff() override;
  void markFullRefresh() override;

private:
  bool _fullPending = true; // first frame is always full
  int _partialsSinceFull = 0;
};
