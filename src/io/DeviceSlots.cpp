#include "DeviceSlots.h"
#include <Preferences.h>

static Preferences prefs;

void DeviceSlots::begin() {
    // Open writable: read-only mode errors out on first boot before the namespace
    // exists. Auto-creating is harmless — we close right after reading.
    prefs.begin("slots", false);
    _active = prefs.getUChar("active", 0);
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        char key[10];
        snprintf(key, sizeof(key), "occ%u", i);
        _slots[i].occupied = prefs.getBool(key, false);
        snprintf(key, sizeof(key), "name%u", i);
        String n = prefs.getString(key, String("Slot ") + (i + 1));
        strlcpy(_slots[i].name, n.c_str(), sizeof(_slots[i].name));
        if (_slots[i].occupied) {
            // Restore address: 6 raw bytes + 1 type byte stored as 7-byte blob.
            uint8_t buf[7] = {};
            snprintf(key, sizeof(key), "addr%u", i);
            if (prefs.getBytes(key, buf, sizeof(buf)) == sizeof(buf)) {
                ble_addr_t raw;
                memcpy(raw.val, buf, 6);
                raw.type = buf[6];
                _slots[i].addr = NimBLEAddress(raw);
            }
        }
    }
    prefs.end();
}

void DeviceSlots::save() {
    prefs.begin("slots", false);
    prefs.putUChar("active", _active);
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        char key[10];
        snprintf(key, sizeof(key), "occ%u", i);
        prefs.putBool(key, _slots[i].occupied);
        snprintf(key, sizeof(key), "name%u", i);
        prefs.putString(key, _slots[i].name);
        snprintf(key, sizeof(key), "addr%u", i);
        if (_slots[i].occupied) {
            // Pack address: 6 raw bytes + type byte.
            uint8_t buf[7];
            memcpy(buf, _slots[i].addr.getVal(), 6);
            buf[6] = _slots[i].addr.getType();
            prefs.putBytes(key, buf, sizeof(buf));
        } else if (prefs.isKey(key)) {
            // Only remove if the key actually exists — remove() on a missing key
            // logs an NVS error even though it's harmless.
            prefs.remove(key);
        }
    }
    prefs.end();
}

void DeviceSlots::setActive(uint8_t i) {
    if (i < SLOT_COUNT) _active = i;
}

int DeviceSlots::findByAddr(const NimBLEAddress& addr) const {
    for (uint8_t i = 0; i < SLOT_COUNT; i++)
        if (_slots[i].occupied && _slots[i].addr == addr)
            return i;
    return -1;
}
