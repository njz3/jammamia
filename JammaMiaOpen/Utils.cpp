/*
  Various utilities
  04/2020 Benjamin Maurin
*/

#include "Utils.h"

#ifdef ARDUINO_AVR_LEONARDO
#include <avr/wdt.h>
#endif

namespace Utils {

static const char nibbletable[] = "0123456789ABCDEFX";

void ConvertToNDigHex(uint32_t value, String& shex, uint32_t N) {
  int32_t i;
  shex.reserve(N);
  char* hex = shex.begin();
  for (i = N - 1; i >= 0; i--) {
    uint32_t nibble = value & 0xF;  // Récupère le nibble 'i'
    hex[i] = nibbletable[nibble];
    value = value >> 4;
  }
  hex[N] = 0;
}

uint32_t ConvertHexToInt(const String& shex, int N) {
  int i;
  uint32_t value = 0;
  const char* hex = shex.c_str();

  for (i = 0; i < N; i++) {
    char valhex;
    if (hex[i] >= '0' && hex[i] <= '9')
      valhex = hex[i] - '0';
    else if (hex[i] >= 'a' && hex[i] <= 'f')
      valhex = hex[i] - 'a' + 0xA;
    else if (hex[i] >= 'A' && hex[i] <= 'F')
      valhex = hex[i] - 'A' + 0xA;
    else {
      // Probably end of string.
      valhex = 0;
      break;
    }
    uint32_t nibble = (uint32_t)(valhex & 0xF);  // Récupère le nibble 'i'
    value = nibble + (value << 4);
  }
  return value;
}
/*
byte ReadByteValue(char *sc) {
  return (uint16_t)Utils::ConvertHexToInt(sc, 2);
}
uint8_t ReadUINT8Value(char *sc) {
  return (uint8_t)Utils::ConvertHexToInt(sc, 2);
}
uint16_t ReadUINT16Value(char *sc) {
  return (uint16_t)Utils::ConvertHexToInt(sc, 4);
}
uint32_t ReadUINT32Value(char *sc) {
  return (uint32_t)Utils::ConvertHexToInt(sc, 8);
}
*/


// Reset function using the avr watchdog
void SoftwareReboot() {
#ifdef ARDUINO_AVR_LEONARDO
  // Use hardware watchdog for Leonardo
  wdt_enable(WDTO_15MS);
  while (1) {}
#endif
#ifdef ARDUINO_AVR_MEGA2560
  // Use branch to address 0 to restart the code
  void (*resetFunc)(void) = 0;  //declare reset function at address 0
  resetFunc();
#endif
#ifdef ESP32
  esp_restart();
#endif
}


/*
// Return remaining string after index or separator
char* GetValue(char* data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = strlen(data) - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data[i] == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : NullStr;
}
*/


// find a token with a given separator and return start position and length of token
// str: input string
// separator: input char for separator (usually ' ')
// tokenIdx: index of token to look for
// length: if token found, length of token in the string. -1 is no token found
// returns: token postion as a pointer in input string
String Token(const String& str, const char separator, int tokenIdx) {
  int stringCount = 0;
  String newstr = String(str);

  if (tokenIdx > 33)  // 32 tokens + 1 for key
    goto _error;

  // Split the string into substrings
  while (str.length() > 0) {
    // Search for separator
    int index = str.indexOf(separator);

    if (index == -1) {
      // No separator found

      // Are we already at the right token?
      if (tokenIdx == stringCount) {
        // Return remaining string
        return newstr;
      }
      goto _error;
    } else {
      // Separator found
      if (index == 0) {
        // Skip empty entries
        index++;
        newstr = newstr.substring(index);
        continue;
      }
      // Retrieve substring in case we are at right token
      if (tokenIdx == stringCount) {
        // split string until next separator
        newstr = newstr.substring(0, index);
        // return token
        return newstr;
      }
      // skip token and advance to next one      
      stringCount++;
      newstr = newstr.substring(index + 1);
    }
  }
_error:
  // It failed, return empty string
  return String();
}

}