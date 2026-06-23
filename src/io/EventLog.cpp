#include "EventLog.h"

#include <cstring>

#include "Clock.h"

namespace {
struct Entry {
  char hhmm[6]; // "HH:MM\0"
  char msg[EventLog::MSG_LEN];
};

Entry _ring[EventLog::CAP] = {};
int _head = 0;  // next write slot
int _count = 0; // 0..CAP
} // namespace

namespace EventLog {

void add(const char *msg) {
  Entry &e = _ring[_head];
  Clock::format(e.hhmm, sizeof(e.hhmm));
  strncpy(e.msg, msg, sizeof(e.msg) - 1);
  e.msg[sizeof(e.msg) - 1] = '\0';

  _head = (_head + 1) % CAP;
  if (_count < CAP)
    _count++;
}

int count() { return _count; }

void get(int i, char *hhmm, char *msg) {
  if (i < 0 || i >= _count) {
    if (hhmm)
      hhmm[0] = '\0';
    if (msg)
      msg[0] = '\0';
    return;
  }
  // Newest = slot just before _head; older entries walk backward.
  int idx = (_head - 1 - i + CAP * 2) % CAP;
  const Entry &e = _ring[idx];
  if (hhmm)
    strncpy(hhmm, e.hhmm, 6);
  if (msg)
    strncpy(msg, e.msg, MSG_LEN);
}

} // namespace EventLog
