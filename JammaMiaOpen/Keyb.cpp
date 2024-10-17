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

static KeyboardNKey_ *Keyboard;

void Setup() {
  Serial.println(F("MKeyboard emulation enabled"));

  Keyboard = new KeyboardNKey_();
  switch (Config::ConfigFile.KeybLayout) {
    case 1:
      Keyboard->begin(KeyboardLayout_fr_FR);
      break;
    case 2:
      Keyboard->begin(KeyboardLayout_de_DE);
      break;
    case 3:
      Keyboard->begin(KeyboardLayout_it_IT);
      break;
    case 4:
      Keyboard->begin(KeyboardLayout_es_ES);
      break;
    case 0:
    default:
      Keyboard->begin(KeyboardLayout_en_US);
      break;
  }
  // Start by clearing all keys just in case
  Keyboard->releaseAll();
}

void Press(byte key) {
  Keyboard->press(key);

#ifdef DEBUG_PRINTF
  Serial.print(F("keyb press: 0x"));
  Serial.println(key, HEX);
#endif
}

void Release(byte key) {
  Keyboard->release(key);

#ifdef DEBUG_PRINTF
  Serial.print(F("keyb release: 0x"));
  Serial.println(key, HEX);
#endif
}

void UpdateToPC() {
  Keyboard->sendState();
#ifdef DEBUG_PRINTF
  Serial.println(F("keyb update"));
#endif
}

}
#endif
