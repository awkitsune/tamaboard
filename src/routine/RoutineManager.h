#pragma once
#include <vector>

#include "Routine.h"

class RoutineManager {
public:
  void add(Routine *r); // registers and calls begin()
  void tick(); // call from loop(); fires each routine on its own interval

private:
  struct Entry {
    Routine *routine;
    uint32_t lastMs;
  };
  std::vector<Entry> _entries;
};
