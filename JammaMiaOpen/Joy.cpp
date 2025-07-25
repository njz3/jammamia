#include "Joy.h"

#ifdef USE_JOY

#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"

#include <Joystick.h>

//#define DEBUG_PRINTF
//#define DEBUG_PRINTF_ANALOG

#define JOYSTICK_COUNT 2

namespace Joy {

static bool StateHasChanged = false;
static Joystick_* pJoystick[JOYSTICK_COUNT] = { nullptr, nullptr };

static byte HATDirections[JOYSTICK_COUNT][4];

void Setup() {
  for (int i = 0; i < JOYSTICK_COUNT; i++) {
    pJoystick[i] = new Joystick_(4 + i, JOYSTICK_TYPE_GAMEPAD,
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

    pJoystick[i]->setXAxisRange(0, 1023);
    pJoystick[i]->setYAxisRange(0, 1023);
    pJoystick[i]->setZAxisRange(0, 1023);
    pJoystick[i]->setRxAxisRange(0, 1023);
    pJoystick[i]->setRyAxisRange(0, 1023);
    pJoystick[i]->setRzAxisRange(0, 1023);
    pJoystick[i]->setRudderRange(0, 1023);
    pJoystick[i]->setThrottleRange(0, 1023);
    pJoystick[i]->begin(false);
  }
}

void BtnPress(byte button) {
  int p = button >> 7;
  int btn = button & 0b01111111;
  if (pJoystick[p] == nullptr)
    return;
  pJoystick[p]->pressButton(btn);
  StateHasChanged = true;

#ifdef DEBUG_PRINTF
  Serial.print(F("Mjoy P"));
  Serial.print(p, HEX);
  Serial.print(F(" press btn "));
  Serial.println(btn, HEX);
#endif
}

void BtnRelease(byte button) {
  int p = button >> 7;
  int btn = button & 0b01111111;
  if (pJoystick[p] == nullptr)
    return;
  pJoystick[p]->releaseButton(btn);
  StateHasChanged = true;
#ifdef DEBUG_PRINTF
  Serial.print(F("Mjoy P"));
  Serial.print(p + 1, HEX);
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
  if (pJoystick[p] == nullptr)
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
  pJoystick[p]->setHatSwitch(hatsw, angle);
  StateHasChanged = true;

#ifdef DEBUG_PRINTF
  Serial.print(F("Mjoy P"));
  Serial.print(p + 1, HEX);
  Serial.print(F(" HAT dir "));
  Serial.print(direction, HEX);
  Serial.print(F(" Angle "));
  Serial.println(angle, HEX);
#endif
}

void SetAxis(byte axis, int16_t value) {
  int p = axis >> 7;
  int16_t signvalue = ((axis & 0b1000) ? JOY_MAXPOS_VAL-value : value);
  byte axisidx = axis & 0b00000111;  // 0..7
  
  if (pJoystick[p] == nullptr)
    return;
  switch (axisidx) {
    case 0:
      pJoystick[p]->setXAxis(signvalue);
      break;
    case 1:
      pJoystick[p]->setYAxis(signvalue);
      break;
    case 2:
      pJoystick[p]->setZAxis(signvalue);
      break;
    case 3:
      pJoystick[p]->setRxAxis(signvalue);
      break;
    case 4:
      pJoystick[p]->setRyAxis(signvalue);
      break;
    case 5:
      pJoystick[p]->setRzAxis(signvalue);
      break;
    case 6:
      pJoystick[p]->setRudder(signvalue);
      break;
    case 7:
      pJoystick[p]->setThrottle(signvalue);
      break;
  }
  StateHasChanged = true;

#ifdef DEBUG_PRINTF_ANALOG
  Serial.print(F("Mjoy P"));
  Serial.print(p + 1, HEX);
  Serial.print(F(" axis "));
  Serial.print(axisidx, HEX);
  Serial.print(F(" value "));
  Serial.println(value, HEX);
#endif
}

void UpdateToPC() {
  if (StateHasChanged) {
    StateHasChanged = false;
    for (int i = 0; i < JOYSTICK_COUNT; i++) {
      if (pJoystick[i] == nullptr)
        continue;
      pJoystick[i]->sendState();
    }
  }
}

}
#endif
