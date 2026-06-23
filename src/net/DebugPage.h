#pragma once
#include <Arduino.h>
#include "../pet/PetFSM.h"
#include "../hid/BleHid.h"
#include "../io/DeviceSlots.h"

String buildDebugPage(PetFSM& fsm, BleHid& ble, DeviceSlots& slots);
