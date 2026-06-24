#pragma once
#include "Canvas.h"

// Status flags drawn as icons to the left of the battery in the header.
void headerSetLocked(bool v);
void headerSetAirplaneMode(bool v);
void headerSetCustomText(const char *txt); // set to nullptr to clear

// Draws a one-line header at the top of the screen and returns the y-offset
// where page content can start.
//
//   idle (carousel):   white bg, black text, separator line
//   entered (in page): black bg, white text — full inversion signals "inside"
//
int drawHeader(Canvas &c, const char *title, bool entered = false);
