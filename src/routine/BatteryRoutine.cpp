#include "BatteryRoutine.h"

#include "../io/Battery.h"

void BatteryRoutine::tick() {
  Battery::sample();
  _broadcaster.pushState();
}
