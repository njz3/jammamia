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
*/

#include "MouseN.h"

#if defined(_USING_HID)

static const uint8_t _hidReportDescriptorMouse1[] PROGMEM = {
  
  //  Mouse
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)  // 54
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x02,                    //     REPORT_ID (2)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0,                          // END_COLLECTION
};

static const uint8_t _hidReportDescriptorMouse2[] PROGMEM = {
  
  //  Mouse
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)  // 54
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x03,                    //     REPORT_ID (3)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0,                          // END_COLLECTION
};

//================================================================================
//================================================================================
//	Mouse

MouseN_::MouseN_(bool isDual)
{
	_isDual = isDual;
    static HIDSubDescriptor node(_hidReportDescriptorMouse1, sizeof(_hidReportDescriptorMouse1));
    HID().AppendDescriptor(&node);
	if (_isDual) {
		static HIDSubDescriptor node(_hidReportDescriptorMouse2, sizeof(_hidReportDescriptorMouse2));
		HID().AppendDescriptor(&node);
	}
}

void MouseN_::begin(void) 
{
}

void MouseN_::end(void) 
{
}

void MouseN_::click(uint8_t b, bool dual)
{
	int index = dual?1:0;
	_mouseReports[index].buttons = b;
	sendReport(dual);
	_mouseReports[index].buttons = 0;
	sendReport(dual);
}

void MouseN_::move(signed char x, signed char y, signed char wheel, bool dual)
{
	int index = dual?1:0;
	_mouseReports[index].x = x;
	_mouseReports[index].y = y;
	_mouseReports[index].wheel = wheel;
	//sendReport(dual);
}

void MouseN_::buttons(uint8_t b, bool dual)
{
	int index = dual?1:0;
	if (b !=_mouseReports[index].buttons)
	{
		_mouseReports[index].buttons = b;
		//move(0,0,0, dual);
	}
}

void MouseN_::press(uint8_t b, bool dual) 
{
	int index = dual?1:0;
	buttons(_mouseReports[index].buttons | b);
}

void MouseN_::release(uint8_t b, bool dual)
{
	int index = dual?1:0;
	buttons(_mouseReports[index].buttons & ~b);
}

bool MouseN_::isPressed(uint8_t b, bool dual)
{
	int index = dual?1:0;
	if ((b & _mouseReports[index].buttons) > 0) 
		return true;
	return false;
}

void MouseN_::sendReport(bool dual)
{
	int report = dual?2:3;
	HID().SendReport(report, &_mouseReports[dual?1:0], 4);
}
  
#endif
