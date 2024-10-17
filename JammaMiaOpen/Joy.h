#pragma once
#include "Config.h"

#ifdef USE_JOY

namespace Joy {

// Directions encoding on 4bits
enum HATDirections : byte {
  Nothing = 0,
  Up = 1<<0,
  Down = 1<<1,
  Right = 1<<2,
  Left = 1<<3,
};

void Setup();
void BtnPress(byte button);
void BtnRelease(byte  button);
void SetAxis(byte axis, int32_t value);
void SetHATSwitch(byte hatdirection, bool enable);
void UpdateToPC();
}

#endif
