#include "Joy.h"

#ifdef USE_JOY

#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"

#include <Joystick.h>

//#define DEBUG_PRINTF

#define JOYSTICK_COUNT 2

namespace Joy {

static Joystick_* Joystick[JOYSTICK_COUNT] = { };

static byte HATDirections[JOYSTICK_COUNT][4];

void Setup() {
  Serial.println(F("MJoystick emulation enabled"));

  for (int i = 0; i < JOYSTICK_COUNT; i++) {
    Joystick[i] = new Joystick_(0x04 + i, JOYSTICK_TYPE_JOYSTICK,
                                Config::ConfigFile.JoyNumberOfButtons,
                                Config::ConfigFile.JoyNumberOfHAT,
                                Config::ConfigFile.JoyNumberOfAxes > 0,  // X
                                Config::ConfigFile.JoyNumberOfAxes > 1,  // Y
                                Config::ConfigFile.JoyNumberOfAxes > 2,  // Z
                                Config::ConfigFile.JoyNumberOfAxes > 3,  // Rx
                                Config::ConfigFile.JoyNumberOfAxes > 4,  // Ry
                                Config::ConfigFile.JoyNumberOfAxes > 5,  // Rz
                                Config::ConfigFile.JoyNumberOfAxes > 6,  // Rudder
                                Config::ConfigFile.JoyNumberOfAxes > 7,  // Throttle
                                false,                                   // Accel
                                false,                                   // Brake
                                false);                                  // Steering
    Joystick[i]->setXAxisRange(0, 1023);
    Joystick[i]->setYAxisRange(0, 1023);
    Joystick[i]->setZAxisRange(0, 1023);
    Joystick[i]->setRxAxisRange(0, 1023);
    Joystick[i]->setRyAxisRange(0, 1023);
    Joystick[i]->setRzAxisRange(0, 1023);
    Joystick[i]->setRudderRange(0, 1023);
    Joystick[i]->setThrottleRange(0, 1023);
    Joystick[i]->begin(false);
  }
}

void BtnPress(byte button) {
  int p = button >> 7;
  int btn = button & 0b01111111;
  if (Joystick[p] == nullptr)
    return;
  Joystick[p]->pressButton(btn);
#ifdef DEBUG_PRINTF
  Serial.print(F("joy P"));
  Serial.print(p, HEX);
  Serial.print(F(" press btn "));
  Serial.println(btn, HEX);
#endif
}

void BtnRelease(byte button) {
  int p = button >> 7;
  int btn = button & 0b01111111;
  if (Joystick[p] == nullptr)
    return;
  Joystick[p]->releaseButton(btn);
#ifdef DEBUG_PRINTF
  Serial.print(F("joy P"));
  Serial.print(p, HEX);
  Serial.print(F(" release btn "));
  Serial.println(btn, HEX);
#endif
}

// UP: 0
// RIGHT: 90
// DOWN: 180
// LEFT: 270
const int16_t DirectionToHATTable[] = {
  -1,   // 0b0000: NO DIRECTION
  0,    // 0b0001: UP
  180,  // 0b0010: DOWN
  -1,   // 0b0011: UP+DOWN IMPOSSIBLE
  270,  // 0b0100: LEFT
  315,  // 0b0101: UP+LEFT
  225,  // 0b0110: DOWN+LEFT
  -1,   // 0b0111: UP+DOWN+LEFT IMPOSSIBLE
  90,   // 0b1000: RIGHT
  45,   // 0b1001: UP+RIGHT
  135,  // 0b1010: DOWN+RIGHT
  -1,   // 0b1011: UP+DOWN+RIGHT IMPOSSIBLE
  -1,   // 0b1100: LEFT+RIGHT IMPOSSIBLE
  -1,   // 0b1101: UP+LEFT+RIGHT IMPOSSIBLE
  -1,   // 0b1110: DOWN+LEFT+RIGHT IMPOSSIBLE
  -1,   // 0b1111: UP+DOWN+LEFT+RIGHT IMPOSSIBLE
};

void SetHATSwitch(byte hatdirection, bool enable) {
  int p = hatdirection >> 7;
  if (Joystick[p] == nullptr)
    return;
  byte hatsw = hatdirection >> 5 & 0b11;
  byte direction = hatdirection & 0b00001111;
  if (enable) {
    // set bit in HATDirections
    HATDirections[p][hatsw] |= direction;
  } else {
    // Clear bit in HATDirections
    HATDirections[p][hatsw] &= ~(direction);
  }
  direction = HATDirections[p][hatsw] & 0b1111;
  int angle = DirectionToHATTable[direction];
  Joystick[p]->setHatSwitch(hatsw, angle);

#ifdef DEBUG_PRINTF
  Serial.print(F("joy P"));
  Serial.print(p, HEX);
  Serial.print(F(" HAT dir "));
  Serial.print(direction, HEX);
  Serial.print(F(" Angle "));
  Serial.println(angle, HEX);
#endif
}

void SetAxis(byte axis, int32_t value) {
  int p = axis >> 7;
  byte axisidx = axis & 0b00000111;  // 0..7
  if (Joystick[p] == nullptr)
    return;
  switch (axisidx) {
    case 0:
      Joystick[p]->setXAxis(value);
      break;
    case 1:
      Joystick[p]->setYAxis(value);
      break;
    case 2:
      Joystick[p]->setZAxis(value);
      break;
    case 3:
      Joystick[p]->setRxAxis(value);
      break;
    case 4:
      Joystick[p]->setRyAxis(value);
      break;
    case 5:
      Joystick[p]->setRzAxis(value);
      break;
    case 6:
      Joystick[p]->setRudder(value);
      break;
    case 7:
      Joystick[p]->setThrottle(value);
      break;
  }

#ifdef DEBUG_PRINTF
  Serial.print(F("joy P"));
  Serial.print(p, HEX);
  Serial.print(F(" axis "));
  Serial.print(axisidx, HEX);
  Serial.print(F(" value "));
  Serial.println(value, HEX);
#endif
}

void UpdateToPC() {
  for (int i = 0; i < JOYSTICK_COUNT; i++) {
    if (Joystick[i] == nullptr)
      continue;
    Joystick[i]->sendState();
  }
#ifdef DEBUG_PRINTF
  Serial.println(F("joy update"));
#endif
}

}
#endif
