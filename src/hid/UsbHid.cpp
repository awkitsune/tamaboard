#include "UsbHid.h"
#include <Arduino.h>

void UsbHid::begin()                             { Serial.println("[UsbHid] unavailable — GPIO19/20 not wired"); }
void UsbHid::end()                               {}
void UsbHid::key(uint8_t, bool)                  {}
void UsbHid::mouseMove(int8_t, int8_t)           {}
void UsbHid::mouseClick(uint8_t)                 {}
