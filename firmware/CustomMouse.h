/*
  Mouse.h

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Updated for working with 16 bit Mouse by Leonid Tsaruk
  GitHub: https://github.com/NightStrang6r/USBHostShieldMouseProxy
*/

#ifndef MOUSE16BIT_h
#define MOUSE16BIT_h

#include "HID.h"

#if !defined(_USING_HID)

#warning "Using legacy HID core (non pluggable)"

#else

//================================================================================
//================================================================================
//  Mouse

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BUTTON4     8
#define MOUSE_BUTTON5     16
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

#include "Structures.h"

extern HIDReportInfo reportInfo;

class Mouse_
{
private:
    uint8_t _buttons;

    uint16_t _x;
    uint16_t _y;
    int8_t _wheel;


    void buttons(uint8_t b);

    HIDReportDescriptor _hidReportDescriptor;

    template <typename T>
    T limit_xy(int16_t xy);

public:
    Mouse_(void);
    void begin(void);
    void end(void);
    void click(uint8_t b = MOUSE_LEFT);
    void move(int16_t x, int16_t y, int8_t wheel = 0);
    void press(uint8_t b = MOUSE_LEFT);     // press LEFT by default
    void release(uint8_t b = MOUSE_LEFT);   // release LEFT by default
    bool isPressed(uint8_t b = MOUSE_LEFT); // check LEFT by default
    void setDescriptor(const HIDReportDescriptor& reportDescriptor);
    void printHIDDescriptor() const;
    void MoveVector2D(int x, int y);
    void Mouse_::Silent(int16_t dx, int16_t dy);
    uint8_t getButtons() const;
};
extern Mouse_ Mouse;

#endif
#endif