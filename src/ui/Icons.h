#pragma once
#include <cstdint>

// XBM-style bitmaps, 1 bit per pixel, MSB-first per byte, row by row.
// Width must be a multiple of 8 for these simple sprites.

// 16x8 battery outline; pass fill level 0..4 to icon_battery_draw().
static constexpr int ICON_BATT_W = 16;
static constexpr int ICON_BATT_H = 8;

// 8x8 bluetooth glyph.
static constexpr int ICON_BT_W = 8;
static constexpr int ICON_BT_H = 8;
extern const uint8_t ICON_BT[ICON_BT_H];

// 8x8 lock glyph.
static constexpr int ICON_LOCK_W = 8;
static constexpr int ICON_LOCK_H = 8;
extern const uint8_t ICON_LOCK[ICON_LOCK_H];

// 8x8 heart glyph.
static constexpr int ICON_HEART_W = 8;
static constexpr int ICON_HEART_H = 8;
extern const uint8_t ICON_HEART[ICON_HEART_H];

// 8x8 airplane glyph (top-down, nose pointing up).
static constexpr int ICON_AIRPLANE_W = 8;
static constexpr int ICON_AIRPLANE_H = 8;
extern const uint8_t ICON_AIRPLANE[ICON_AIRPLANE_H];
