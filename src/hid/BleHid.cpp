#include "BleHid.h"
#include <Arduino.h>

// HID report descriptor: boot-compatible keyboard (8 bytes) + mouse (4 bytes).
// Two separate report IDs so the host can distinguish them.
static const uint8_t HID_REPORT_MAP[] = {
    // Keyboard — report ID 1
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID 1
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (224)
    0x29, 0xE7,       //   Usage Maximum (231)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute) — modifier byte
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Constant) — reserved byte
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data, Array, Absolute) — keycodes[6]
    0xC0,             // End Collection
    // Mouse — report ID 2
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID 2
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    0x05, 0x09,       //     Usage Page (Buttons)
    0x19, 0x01,       //     Usage Minimum (1)
    0x29, 0x03,       //     Usage Maximum (3)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x95, 0x03,       //     Report Count (3)
    0x75, 0x01,       //     Report Size (1)
    0x81, 0x02,       //     Input (Data, Variable, Absolute) — buttons
    0x95, 0x01,       //     Report Count (1)
    0x75, 0x05,       //     Report Size (5)
    0x81, 0x01,       //     Input (Constant) — padding
    0x05, 0x01,       //     Usage Page (Generic Desktop)
    0x09, 0x30,       //     Usage (X)
    0x09, 0x31,       //     Usage (Y)
    0x15, 0x81,       //     Logical Minimum (-127)
    0x25, 0x7F,       //     Logical Maximum (127)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x02,       //     Report Count (2)
    0x81, 0x06,       //     Input (Data, Variable, Relative) — X, Y
    0xC0,             //   End Collection
    0xC0,             // End Collection
};

BleHid::BleHid(DeviceSlots& slots) : _slots(slots) {}

void BleHid::begin() {
    if (_active) return;
    _active = true;

    if (!_server) {
        // First call: bring up the NimBLE stack and register services.
        // Subsequent begin() calls reuse the live stack — deinit() races with
        // the NimBLE host task on ESP32-S3 and causes InstrFetchProhibited panics.
        NimBLEDevice::init("Tamaboard");
        NimBLEDevice::setSecurityAuth(true, true, true);          // bond, MITM, SC
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
        NimBLEDevice::setMTU(64);

        _server = NimBLEDevice::createServer();
        _server->setCallbacks(this);

        _hid = new NimBLEHIDDevice(_server);
        _hid->setManufacturer("Tamaboard");
        _hid->setPnp(0x02, 0x045E, 0x07A5, 0x0111);             // USB HID, random VID/PID
        _hid->setHidInfo(0x00, 0x01);
        _hid->setReportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));

        _keyReport   = _hid->getInputReport(1);
        _mouseReport = _hid->getInputReport(2);

        _hid->setBatteryLevel(100);
        _server->start();              // NimBLE 2.x starts services automatically

        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        adv->setAppearance(0x03C1);   // keyboard
        adv->addServiceUUID(_hid->getHidService()->getUUID());
    }

    _startAdvertising();
}

void BleHid::end() {
    if (!_active) return;
    _active = false;
    NimBLEDevice::stopAdvertising();
    if (_server && _server->getConnectedCount())
        _server->disconnect(0);
    // Do NOT call NimBLEDevice::deinit() — it signals the host task to stop but
    // returns before the task has actually exited. Any pending event processed
    // after teardown jumps to a null callback and panics (InstrFetchProhibited,
    // PC=0x00000000). The stack stays live; begin() skips re-init on _server != nullptr.
    _setStatus(BleHidStatus::IDLE);
}

void BleHid::key(uint8_t keycode, bool down) {
    if (!_keyReport || _status != BleHidStatus::CONNECTED) return;

    if (keycode >= 0xE0 && keycode <= 0xE7) {
        // Modifier key (L/R Ctrl/Shift/Alt/GUI): update the modifier byte rather
        // than placing the code in the keycode array — that's the HID spec.
        uint8_t bit = (uint8_t)(1u << (keycode - 0xE0));
        if (down) _modifiers |= bit;
        else      _modifiers &= (uint8_t)~bit;
        uint8_t report[8] = { _modifiers, 0, 0, 0, 0, 0, 0, 0 };
        _keyReport->setValue(report, sizeof(report));
        _keyReport->notify();
        return;
    }

    // Regular key: modifier byte stays set so modifier+key combos work.
    uint8_t report[8] = { _modifiers, 0, 0, 0, 0, 0, 0, 0 };
    if (down) report[2] = keycode;
    _keyReport->setValue(report, sizeof(report));
    _keyReport->notify();
    // One notification per call. The web UI controls timing via pointerdown/pointerup.
}

void BleHid::mouseMove(int8_t dx, int8_t dy) {
    if (!_mouseReport || _status != BleHidStatus::CONNECTED) return;
    uint8_t report[4] = { 0, (uint8_t)dx, (uint8_t)dy, 0 };
    _mouseReport->setValue(report, sizeof(report));
    _mouseReport->notify();
}

void BleHid::mouseClick(uint8_t button) {
    if (!_mouseReport || _status != BleHidStatus::CONNECTED) return;
    // button=0 means button-up (all released); any other value sets those button bits.
    // The web UI sends down on pointerdown and up (btn=0) on pointerup — one notify each.
    uint8_t report[4] = { button, 0, 0, 0 };
    _mouseReport->setValue(report, sizeof(report));
    _mouseReport->notify();
}

void BleHid::selectSlot(uint8_t i) {
    if (i >= SLOT_COUNT || !_slots[i].occupied) return;
    _slots.setActive(i);
    _slots.save();
    if (_server && _server->getConnectedCount())
        _server->disconnect(0);
    _startAdvertising();
}

void BleHid::pairInto(uint8_t i) {
    if (i >= SLOT_COUNT) return;
    if (_slots[i].occupied)
        NimBLEDevice::deleteBond(_slots[i].addr);
    _slots[i].occupied = false;
    _slots.save();
    _pairingSlot = i;
    _slots.setActive(i);
    if (_server && _server->getConnectedCount())
        _server->disconnect(0);
    _startAdvertising();   // open advertising so any host can connect
}

void BleHid::forgetSlot(uint8_t i) {
    if (i >= SLOT_COUNT) return;
    if (_slots[i].occupied) {
        NimBLEDevice::deleteBond(_slots[i].addr);
        _slots[i].occupied = false;
        strlcpy(_slots[i].name,
                (String("Slot ") + (i + 1)).c_str(),
                sizeof(_slots[i].name));
        _slots.save();
    }
    if (_status == BleHidStatus::CONNECTED && _slots.active() == i)
        _server->disconnect(0);
}

void BleHid::_startAdvertising() {
    if (!_hid) { Serial.println("[BleHid] _startAdvertising called before begin()"); return; }
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->stop();
    adv->clearData();
    adv->setAppearance(0x03C1);
    adv->addServiceUUID(_hid->getHidService()->getUUID());
    // Always undirected. Directed advertising (with peer addr) times out in 1.28 s
    // and silently stops; macOS also uses Resolvable Private Addresses that change
    // over time, so the stored peer addr quickly becomes stale.
    adv->start(0);
    _setStatus(BleHidStatus::ADVERTISING);
    Serial.println("[BleHid] advertising");
}

void BleHid::_setStatus(BleHidStatus s) {
    _status = s;
    if (_statusCallback) _statusCallback(s);
}

// --- NimBLEServerCallbacks ---

void BleHid::onConnect(NimBLEServer*, NimBLEConnInfo& info) {
    if (!_active) { _server->disconnect(info.getConnHandle()); return; }
    Serial.printf("[BleHid] connected: %s\n", info.getAddress().toString().c_str());
    _setStatus(BleHidStatus::PAIRING);
}

void BleHid::onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) {
    Serial.printf("[BleHid] disconnected, reason=%d\n", reason);
    if (!_active) return;   // end() was called — don't re-advertise
    _startAdvertising();
}

void BleHid::onAuthenticationComplete(NimBLEConnInfo& info) {
    if (!_active) return;
    if (!info.isEncrypted()) {
        Serial.println("[BleHid] auth failed");
        _server->disconnect(info.getConnHandle());
        _setStatus(BleHidStatus::FAILED);
        return;
    }
    Serial.println("[BleHid] bonded/connected");
    // Persist new bond into the pairing slot if this was a fresh pair.
    int existing = _slots.findByAddr(info.getAddress());
    if (existing < 0) {
        // New bond — store into _pairingSlot.
        _slots[_pairingSlot].occupied = true;
        _slots[_pairingSlot].addr     = info.getAddress();
        _slots.save();
    }
    _setStatus(BleHidStatus::CONNECTED);
    if (_passkeyCallback) _passkeyCallback(0);   // 0 = paired, clear PIN screen
}

uint32_t BleHid::onPassKeyDisplay() {
    uint32_t pk = 100000 + (esp_random() % 900000);
    Serial.printf("[BleHid] PIN: %06lu\n", (unsigned long)pk);
    if (_passkeyCallback) _passkeyCallback(pk);
    return pk;
}
