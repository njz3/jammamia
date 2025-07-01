#include "Mou.h"
#ifdef USE_MOUSE

#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"

#ifdef ARDUINO_AVR_LEONARDO
#include <MouseN.h>
#else
#error No support
#endif

#define DEBUG_PRINTF

namespace Mou {

static MouseN_ *Mouse = nullptr;

const uint8_t MouseButtons[] = { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE };

void Setup() {
#ifdef DEBUG_PRINTF
  Serial.println(F("MMouse emulation enabled"));
#endif
  Mouse = new MouseN_(true);
  Mouse->begin();
}

void BtnPress(byte button) {
  int p = button >> 7;
  int btn = button & 0b111;
  if (Mouse == nullptr)
    return;
  // if the mouse is not pressed, press it:
  if (!Mouse->isPressed(MouseButtons[btn], p == 1)) {
    Mouse->press(MouseButtons[btn], p == 1);
  }
#ifdef DEBUG_PRINTF
  Serial.print(F("mouse P"));
  Serial.print(p, HEX);
  Serial.print(F(" press btn "));
  Serial.println(btn, HEX);
#endif
}

void BtnRelease(byte button) {
  int p = button >> 7;
  int btn = button & 0b111;
  if (Mouse == nullptr)
    return;
  // if the mouse is pressed, release it:
  if (Mouse->isPressed(MouseButtons[btn], p == 1)) {
    Mouse->release(MouseButtons[btn], p == 1);
  }
#ifdef DEBUG_PRINTF
  Serial.print(F("joy P"));
  Serial.print(p, HEX);
  Serial.print(F(" release btn "));
  Serial.println(btn, HEX);
#endif
}

void SetAxis(byte axis, int32_t value) {
  int p = axis >> 7;
  int dir = axis & 0b111;

  int xDistance = (dir == 0b001 ? value : 0);
  int yDistance = (dir == 0b010 ? value : 0);
  int wDistance = (dir == 0b100 ? value : 0);

  if (Mouse == nullptr)
    return;
  Mouse->move(xDistance, yDistance, wDistance, p == 1);
}


void IncrAxis(byte axis, int32_t value) {
  int p = axis >> 7;
  int dir = axis & 0b111;

  int xDistance = (dir == 0b001 ? value : 0);
  int yDistance = (dir == 0b010 ? value : 0);
  int wDistance = (dir == 0b100 ? value : 0);

  if (Mouse == nullptr)
    return;
  Mouse->move(xDistance, yDistance, wDistance, p == 1);
}


void UpdateToPC() {
  if (Mouse == nullptr)
    return;
  Mouse->sendReport(false);
  Mouse->sendReport(true);
#ifdef DEBUG_PRINTF
  //Serial.println(F("mouse update"));
#endif
}

}
#endif
