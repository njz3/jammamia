/*
  CRC computations
*/
#pragma once
#include "Config.h"

namespace CRC {
  
uint8_t crc8(const void *mem, uint16_t len, uint8_t prevCrc8 = 0);
}
