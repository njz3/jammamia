#pragma once
#include "Config.h"

#ifdef USE_MOUSE

namespace Mou {
void Setup();
void BtnPress(byte button);
void BtnRelease(byte button);
void SetAxis(byte axis, int32_t value);
void IncrAxis(byte axis, int32_t value);
void UpdateToPC();
}

#endif
