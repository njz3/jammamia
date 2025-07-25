#pragma once
#include "Config.h"

#ifdef USE_MOUSE

namespace Mou {
void Setup();
void BtnPress(byte button);
void BtnRelease(byte button);
void MoveAxis(byte axis, int8_t value);
void UpdateToPC();
}

#endif
