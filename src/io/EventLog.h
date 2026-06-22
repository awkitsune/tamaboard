#pragma once

// Small ring buffer of recent FSM events ("Fed", "Played", ...) stamped with
// the current Clock::format() HH:MM. Rendered at the bottom of HomePage so the
// user can glance at the last few care actions. Device-side only for now —
// not broadcast to the companion.
namespace EventLog
{
    static constexpr int CAP = 3;
    static constexpr int MSG_LEN = 24; // longest message including NUL

    void add(const char *msg);
    int count(); // 0..CAP
    // Copies the i-th most recent entry into the caller's buffers.
    // i == 0 → newest; hhmm must hold 6 bytes ("HH:MM\0"), msg must hold MSG_LEN.
    void get(int i, char *hhmm, char *msg);
}
