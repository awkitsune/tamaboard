#pragma once
#include "../net/StateBroadcaster.h"
#include "Routine.h"

class BatteryRoutine : public Routine {
public:
  explicit BatteryRoutine(StateBroadcaster &broadcaster)
      : _broadcaster(broadcaster) {}
  void tick() override;
  uint32_t intervalMs() const override { return 60000; }

private:
  StateBroadcaster &_broadcaster;
};
