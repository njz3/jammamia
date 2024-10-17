/*
  Various utilities
  04/2020 Benjamin Maurin
*/
#pragma once
#include "Config.h"

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


namespace Utils {

void ConvertToNDigHex(uint32_t value, String& hex, uint32_t N = 2);
uint32_t ConvertHexToInt(const String& hex, int N = 2);
/*
byte ReadByteValue(const String& sc);
uint8_t ReadUINT8Value(const String& sc);
uint16_t ReadUINT16Value(const String& sc);
uint32_t ReadUINT32Value(const String& sc);
*/
void SoftwareReboot();
//char[] GetValue(char data[], char separator, int index);
String Token(const String &str, const char separator, int index);

}
