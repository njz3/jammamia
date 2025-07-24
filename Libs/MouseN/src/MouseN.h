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
*/

#ifndef MOUSEN_h
#define MOUSEN_h

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
#define MOUSE_PREV 8
#define MOUSE_NEXT 16
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE | MOUSE_PREV | MOUSE_NEXT)

// Low level mouse report: buttons follow by X/Y change
typedef struct __attribute__((__packed__))
{
  uint8_t buttons;
  signed char x;
  signed char y;
  signed char wheel;
} MouseReport;


class MouseN_
{
private:
  bool _isDual;
  MouseReport _mouseReports[2];
  void buttons(uint8_t b, bool dual = false);
public:
  MouseN_(bool isDual);
  void begin(void);
  void end(void);
  void click(uint8_t b = MOUSE_LEFT, bool dual = false);
  void move(signed char x, signed char y, signed char wheel = 0, bool dual = false); 
  void press(uint8_t b = MOUSE_LEFT, bool dual = false);   // press LEFT by default
  void release(uint8_t b = MOUSE_LEFT, bool dual = false); // release LEFT by default
  bool isPressed(uint8_t b = MOUSE_LEFT, bool dual = false); // check LEFT by default
  void sendReport(bool dual);
};

#endif
#endif