#pragma once
#include <cstdint>

class AsyncWebSocket;
class AsyncWebSocketClient;
class PetFSM;
class BleHid;
class DeviceSlots;

// Builds JSON snapshots and ws.textAll()s them. Single source of all server-push
// traffic — pages and controllers don't talk to the socket directly.
class StateBroadcaster {
public:
    StateBroadcaster(AsyncWebSocket& ws,
                     PetFSM& fsm, BleHid& ble, DeviceSlots& slots)
        : _ws(ws), _fsm(fsm), _ble(ble), _slots(slots) {}

    // Broadcast snapshots to all connected clients.
    void pushState();
    void pushSlots();
    void pushPairing(uint32_t passkey);
    void pushAll();

    // Send snapshots to a single just-connected client.
    void sendInitial(AsyncWebSocketClient& client);

private:
    AsyncWebSocket& _ws;
    PetFSM&         _fsm;
    BleHid&         _ble;
    DeviceSlots&    _slots;
};
