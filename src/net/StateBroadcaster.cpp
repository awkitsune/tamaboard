#include "StateBroadcaster.h"
#include "../pet/PetFSM.h"
#include "../hid/BleHid.h"
#include "../io/DeviceSlots.h"
#include "../io/Battery.h"
#include "../io/Clock.h"
#include <ESPAsyncWebServer.h>

static const char* stateName(State s) {
    switch (s) {
        case State::IDLE:     return "IDLE";
        case State::SLEEP:    return "SLEEP";
        case State::EAT:      return "EAT";
        case State::PLAY:     return "PLAY";
        case State::KEYBOARD: return "KEYBOARD";
    }
    return "?";
}
static const char* bleName(BleHidStatus s) {
    switch (s) {
        case BleHidStatus::IDLE:        return "IDLE";
        case BleHidStatus::ADVERTISING: return "ADVERTISING";
        case BleHidStatus::PAIRING:     return "PAIRING";
        case BleHidStatus::CONNECTED:   return "CONNECTED";
        case BleHidStatus::FAILED:      return "FAILED";
    }
    return "?";
}

static String buildState(PetFSM& fsm, BleHid& ble, DeviceSlots& slots) {
    const Pet& p = fsm.pet();
    uint8_t a = slots.active();
    const auto& sl = slots[a];
    String j; j.reserve(220);
    j += "{\"t\":\"state\",";
    j += "\"state\":\""; j += stateName(fsm.state()); j += "\",";
    j += "\"hunger\":";  j += p.hunger; j += ",";
    j += "\"mood\":";    j += p.mood;   j += ",";
    j += "\"energy\":";  j += p.energy; j += ",";
    j += "\"battery\":"; j += Battery::percent(); j += ",";
    j += "\"ble\":\"";   j += bleName(ble.status()); j += "\",";
    j += "\"ble_slot\":";j += a; j += ",";
    j += "\"ble_slot_name\":\""; j += (sl.occupied ? sl.name : ""); j += "\",";
    j += "\"clock_synced\":"; j += (Clock::synced() ? "true" : "false");
    j += "}";
    return j;
}

static String buildSlots(BleHid& ble, DeviceSlots& slots) {
    String j; j.reserve(260);
    j += "{\"t\":\"slots\",\"active\":"; j += slots.active(); j += ",\"slots\":[";
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        const auto& sl = slots[i];
        bool connected = (slots.active() == i
                          && ble.status() == BleHidStatus::CONNECTED);
        if (i) j += ",";
        j += "{\"name\":\""; j += sl.name; j += "\",";
        j += "\"bonded\":";    j += (sl.occupied ? "true" : "false"); j += ",";
        j += "\"connected\":"; j += (connected   ? "true" : "false"); j += "}";
    }
    j += "]}";
    return j;
}

static String buildPairing(uint32_t pk) {
    String j; j.reserve(40);
    j += "{\"t\":\"pairing\",\"passkey\":"; j += pk; j += "}";
    return j;
}

void StateBroadcaster::pushState()                  { _ws.textAll(buildState(_fsm, _ble, _slots)); }
void StateBroadcaster::pushSlots()                  { _ws.textAll(buildSlots(_ble, _slots)); }
void StateBroadcaster::pushPairing(uint32_t pk)     { _ws.textAll(buildPairing(pk)); }
void StateBroadcaster::pushAll()                    { pushState(); pushSlots(); }

void StateBroadcaster::sendInitial(AsyncWebSocketClient& client) {
    client.text(buildState(_fsm, _ble, _slots));
    client.text(buildSlots(_ble, _slots));
}
