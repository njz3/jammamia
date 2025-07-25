#include "Keyb.h"
#ifdef USE_KEYB

#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"

#include <KeyboardNKey.h>

//#define DEBUG_PRINTF

namespace Keyb {

static bool StateHasChanged = false;
static KeyboardNKey_ *pKeyboard = nullptr;

void Setup() {
  pKeyboard = new KeyboardNKey_();
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
}

void Press(byte key) {
  if ((pKeyboard == nullptr) || (key == 0))
    return;
  pKeyboard->press(key);
  StateHasChanged = true;
#ifdef DEBUG_PRINTF
  Serial.print(F("Mkeyb press: 0x"));
  Serial.println(key, HEX);
#endif
}

void Release(byte key) {
  if ((pKeyboard == nullptr) || (key == 0))
    return;
  pKeyboard->release(key);
  StateHasChanged = true;
#ifdef DEBUG_PRINTF
  Serial.print(F("Mkeyb release: 0x"));
  Serial.println(key, HEX);
#endif
}

void UpdateToPC() {
  if (pKeyboard == nullptr)
    return;
  if (StateHasChanged) {
    pKeyboard->sendState();
    StateHasChanged = false;
  }
#ifdef DEBUG_PRINTF
  //Serial.println(F("keyb update"));
#endif
}

}
#endif
