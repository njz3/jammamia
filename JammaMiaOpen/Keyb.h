#pragma once
#include "Config.h"

#ifdef USE_KEYB

namespace Keyb {

void Setup();
void Press(byte key);
void Release(byte key);
void UpdateToPC();

}

#endif
