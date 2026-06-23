#pragma once
#include <cstdint>

class Routine {
public:
  virtual void begin() {}
  virtual void tick() = 0;
  virtual uint32_t intervalMs() const = 0;
  virtual ~Routine() = default;
};
