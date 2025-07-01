#include "Keyb.h"
#ifdef USE_KEYB

#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"

#ifdef ARDUINO_AVR_LEONARDO
#include <KeyboardNKey.h>
#else
#error No support
#endif

//#define DEBUG_PRINTF

namespace Keyb {

static KeyboardNKey_ *pKeyboard = nullptr;

void Setup() {
#ifdef DEBUG_PRINTF
  Serial.println(F("MKeyboard emulation enabled"));
#endif

  pKeyboard = &Keyboard;
  switch (Config::ConfigFile.KeybLayout) {
    case 1:
      pKeyboard->begin(KeyboardLayout_fr_FR);
      break;
    case 2:
      pKeyboard->begin(KeyboardLayout_de_DE);
      break;
    case 3:
      pKeyboard->begin(KeyboardLayout_it_IT);
      break;
    case 4:
      pKeyboard->begin(KeyboardLayout_es_ES);
      break;

    case 0:
    default:
      pKeyboard->begin(KeyboardLayout_en_US);
      break;
  }
  // Start by clearing all keys just in case
  pKeyboard->releaseAll();
}

void Press(byte key) {
  if (pKeyboard == nullptr)
    return;
  pKeyboard->press(key);

#ifdef DEBUG_PRINTF
  Serial.print(F("keyb press: 0x"));
  Serial.println(key, HEX);
#endif
}

void Release(byte key) {
  if (pKeyboard == nullptr)
    return;
  pKeyboard->release(key);

#ifdef DEBUG_PRINTF
  Serial.print(F("keyb release: 0x"));
  Serial.println(key, HEX);
#endif
}

void UpdateToPC() {
  if (pKeyboard == nullptr)
    return;
  pKeyboard->sendState();
#ifdef DEBUG_PRINTF
  Serial.println(F("keyb update"));
#endif
}

}
#endif
