#include "RoutineManager.h"
#include <Arduino.h>

void RoutineManager::add(Routine* r) {
    r->begin();
    _entries.push_back({r, millis()});
}

void RoutineManager::tick() {
    uint32_t now = millis();
    for (auto& e : _entries) {
        if (now - e.lastMs >= e.routine->intervalMs()) {
            e.lastMs = now;
            e.routine->tick();
        }
    }
}
