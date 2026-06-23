#include "Icons.h"

// Hand-drawn 8x8 pixel glyphs. Each byte = one row, MSB = leftmost pixel.

const uint8_t ICON_BT[ICON_BT_H] = {
    0b00010000,
    0b00011100,
    0b01010010,
    0b00110100,
    0b00011000,
    0b00110100,
    0b01010010,
    0b00011100,
};

const uint8_t ICON_LOCK[ICON_LOCK_H] = {
    0b00111100,
    0b01000010,
    0b01000010,
    0b11111111,
    0b11100111,
    0b11101111,
    0b11111111,
    0b00000000,
};

const uint8_t ICON_HEART[ICON_HEART_H] = {
    0b01100110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000,
};

// Top-down airplane: fuselage at center, wide wings at row 2, narrow tail at row 5.
const uint8_t ICON_AIRPLANE[ICON_AIRPLANE_H] = {
    0b00001000,
    0b00001100,
    0b11001100,
    0b01111111,
    0b00001100,
    0b00001100,
    0b00001000,
    0b00000000,
};
