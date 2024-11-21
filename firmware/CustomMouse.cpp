/*
  Mouse.cpp

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

#include "CustomMouse.h"

#if defined(_USING_HID)

static const uint8_t _tmpHidReportDescriptor[] PROGMEM = {
    0x05, 0x01,       /* Usage Page (Generic Desktop),       */
    0x09, 0x02,       /* Usage (Mouse),                      */
    0xA1, 0x01,       /*  Collection (Application),          */
    0x85, 0x01,       /*     Report Id 1  Mouse Motion       */
    0x09, 0x01,       /*   Usage (Pointer),                  */
    0xA1, 0x00,       /*  Collection (Physical),             */
    0x05, 0x09,       /*     Usage Page (Buttons),           */
    0x19, 0x01,       /*     Usage Minimum (01),             */
    0x29, 0x05,       /*     Usage Maximum (05),  5 buttons  */
    0x15, 0x00,       /*     Logical Minimum (0),            */
    0x25, 0x01,       /*     Logical Maximum (1),            */
    0x75, 0x01,       /*     Report Size (1),                */
    0x95, 0x05,       /*     Report Count (5),               */
    0x81, 0x02,       /*     Input (Data, Variable, Absolute)*/
    0x75, 0x03,       /*     Report Size (3), 3 bit padding  */
    0x95, 0x01,       /*     Report Count (1),               */
    0x81, 0x01,       /*     Input (Constant),               */
    0x05, 0x01,       /*     Usage Page (Generic Desktop),   */
    0x09, 0x30,       /*     Usage (X),                      */
    0x09, 0x31,       /*     Usage (Y),                      */
    0x16, 0x01, 0x80, /*     Logical Minimum (-32767),       */
    0x26, 0xFF, 0x7F, /*     Logical Maximum (32767),        */
    0x75, 0x10,       /*     Report Size (16),               */
    0x95, 0x02,       /*     Report Count (2),               */
    0x81, 0x06,       /*     Input (Data, Variable, Relative)*/
    0x09, 0x38,       /*     Usage (Scroll),                 */
    0x15, 0x81,       /*     Logical Minimum(-127)           */
    0x25, 0x7F,       /*     Logical Maximum(127)            */
    0x75, 0x08,       /*     Report Size(8)                  */
    0x95, 0x01,       /*     Report Count(1)                 */
    0x81, 0x06,       /*     Input(Data,Var,Rel)             */
    0xC0,             /*  End Collection,                    */
    0xC0,             /* End Collection                      */
};

//================================================================================
//================================================================================
//	Mouse

#define LOGICAL_MAXIMUM		    127
#define LOGICAL_MINIMUM		    -127

/* Define a template since we don't know if we are dealing with 8 or 16 bit*/
template <typename T>
T Mouse_::limit_xy(int16_t xy)
{
    if (reportInfo.xySize == 8) {
        if (xy < -127)       return static_cast<T>(-127);
        else if (xy > 127)   return static_cast<T>(127);
        else                 return static_cast<T>(xy);
    }
    else {
        if (xy < -32767)     return static_cast<T>(-32767);
		else if (xy > 32767) return static_cast<T>(32767);
		else                 return static_cast<T>(xy);
    }
}

Mouse_::Mouse_(void) : _buttons(0), _x(0), _y(0), _wheel(0)
{
    static HIDSubDescriptor node(_tmpHidReportDescriptor, sizeof(_tmpHidReportDescriptor));
    HID().AppendDescriptor(&node);

    /*static HIDSubDescriptor vendorNode(vendorHidReportDescriptor, sizeof(vendorHidReportDescriptor));
    HID().AppendDescriptor(&vendorNode);*/
}

void Mouse_::begin(void)
{
}

void Mouse_::end(void)
{
}

void Mouse_::click(uint8_t b)
{
    uint8_t previousButtons = _buttons; // Preserve the current button state
    _buttons |= b;
    move(0, 0, 0);
    _buttons = previousButtons;  // Restore the previous button state
    move(0, 0, 0);
}
void Mouse_::move(int16_t x, int16_t y, int8_t wheel)
{
    /* Move function remains the same for both 16 bit & 8 bit mice */

    /*_x = x;
    _y = y;
    _wheel = wheel;
    */

    uint8_t m[6]; // every element in this array is 8 bits. We seperate the high byte / low byte into different buffer zones
    m[0] = _buttons;
    m[1] = limit_xy<uint16_t>(x) & 0xff;
    m[2] = (limit_xy<uint16_t>(x) >> 8) & 0xff;
    m[3] = limit_xy<uint16_t>(y) & 0xff;
    m[4] = (limit_xy<uint16_t>(y) >> 8) & 0xff;
    m[5] = wheel;
    HID().SendReport(1, m, 6);
}

void Mouse_::MoveVector2D(int x, int y) {

    if (x != 0) {
        while (x > LOGICAL_MAXIMUM || x < LOGICAL_MINIMUM) {
            int moveX = (x > 0) ? LOGICAL_MAXIMUM : LOGICAL_MINIMUM;
            move(moveX, 0);
            x -= moveX;
        }
        move(x, 0);
    }

    if (y != 0) {
        while (y > LOGICAL_MAXIMUM || y < LOGICAL_MINIMUM) {
            int moveY = (y > 0) ? LOGICAL_MAXIMUM : LOGICAL_MINIMUM;
            move(0, moveY);
            y -= moveY;
        }
        move(0, y);
    }
}

//Vendor specific move function
//void Mouse_::move(int16_t x, int16_t y, int8_t wheel)
//{
//    /* Move function to match the HID descriptor */
//
//    uint8_t m[7]; 
//
//    // Buttons
//    m[0] = _buttons & 0xFF; 
//    m[1] = (_buttons >> 8) & 0xFF;
//
//    // X axis
//    m[2] = limit_xy<int16_t>(x) & 0xFF;
//    m[3] = (limit_xy<int16_t>(x) >> 8) & 0xFF;
//
//    // Y axis
//    m[4] = limit_xy<int16_t>(y) & 0xFF;
//    m[5] = (limit_xy<int16_t>(y) >> 8) & 0xFF;
//
//    // Wheel
//    m[6] = static_cast<uint8_t>(wheel);
//
//    HID().SendReport(1, m, 7);
//}

uint8_t Mouse_::getButtons() const
{
    return _buttons;
}

void Mouse_::buttons(uint8_t b)
{
    if (b != _buttons)
    {
        _buttons = b;
        move(0, 0, 0);
    }
}

void Mouse_::press(uint8_t b)
{
    buttons(_buttons | b);
}

void Mouse_::release(uint8_t b)
{
    buttons(_buttons & ~b);
}

void Mouse_::Silent(int16_t dx, int16_t dy)
{
    int16_t dxn = dx * -1;
    int16_t dyn = dy * -1;

    if (dx > 0) {
        while (dx > 127)
        {
            dx -= 127;
            move(127, 0);
        }
        move(dx, 0);
    }
    else if (dx < 0) {
        while (dx < -127)
        {
            dx += 127;
            move(-127, 0);
        }
        move(dx, 0);
    }
    if (dy > 0) {
        while (dy > 127)
        {
            dy -= 127;
            move(0, 127);
        }
        move(0, dy);
    }
    else if (dy < 0) {
        while (dy < -127)
        {
            dy += 127;
            move(0, -127);
        }
        move(0, dy);
    }
    click();
    if (dxn > 0) {
        while (dxn > 127)
        {
            dxn -= 127;
            move(127, 0);
        }
        move(dxn, 0);
    }
    else if (dxn < 0) {
        while (dxn < -127)
        {
            dxn += 127;
            move(-127, 0);
        }
        move(dxn, 0);
    }
    if (dyn > 0) {
        while (dyn > 127)
        {
            dyn -= 127;
            move(0, 127);
        }
        move(0, dyn);
    }
    else if (dyn < 0) {
        while (dyn < -127)
        {
            dyn += 127;
            move(0, -127);
        }
        move(0, dyn);
    }
}


bool Mouse_::isPressed(uint8_t b)
{
    if ((b & _buttons) > 0)
        return true;
    return false;
}

void Mouse_::setDescriptor(const HIDReportDescriptor& reportDescriptor) {
    this->_hidReportDescriptor = reportDescriptor;
    static HIDSubDescriptor node(reportDescriptor.descriptor, reportDescriptor.size);
    HID().AppendDescriptor(&node);
}

void Mouse_::printHIDDescriptor() const {
    Serial.println("Current Mouse HID Report Descriptor:");
    for (uint16_t i = 0; i < this->_hidReportDescriptor.size; i++) {
        Serial.print(this->_hidReportDescriptor.descriptor[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

Mouse_ Mouse;

#endif