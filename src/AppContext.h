#pragma once

class PetFSM;
class BleHid;
class DeviceSlots;
class PageStack;
class Controller;
class Led;

// What pages may read (fsm, slots, ble for status) vs. what they may write
// (controller). The split matches CQRS-flavored intent: pages observe state
// directly, but never mutate it without going through the Controller.
struct AppContext {
  PetFSM &fsm;
  BleHid &ble;        // read-only access (status, for KeyboardPage badge)
  DeviceSlots &slots; // read-only access (slot names, etc.)
  Controller &ctrl;   // all mutations go through here
  PageStack &nav;
  Led &led;
};
