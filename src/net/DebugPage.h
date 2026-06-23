#pragma once
#include <Arduino.h>

#include "../hid/BleHid.h"
#include "../io/DeviceSlots.h"
#include "../pet/PetFSM.h"

String buildDebugPage(PetFSM &fsm, BleHid &ble, DeviceSlots &slots);
