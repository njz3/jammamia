#include "Mou.h"
#ifdef USE_MOUSE

#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"

#include <MouseN.h>

//#define DEBUG_PRINTF

namespace Mou {

static bool ButtonStateHasChanged = false;
static bool MoveStateHasChanged = false;

static MouseN_ *pMouse = nullptr;

const uint8_t MouseButtons[] = { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_PREV, MOUSE_NEXT, 0, 0, 0 };

void Setup() {
  pMouse = new MouseN_(true);
  pMouse->begin();
}

void BtnPress(byte button) {
  int p = button >> 7;
  int btn = button & 0b111;  // up to 5 buttons
  if (pMouse == nullptr)
    return;
  // if the mouse is not pressed, press it:
  if (!pMouse->isPressed(MouseButtons[btn], p == 1)) {
    pMouse->press(MouseButtons[btn], p == 1);
  }
  ButtonStateHasChanged = true;
#ifdef DEBUG_PRINTF
  Serial.print(F("Mmouse P"));
  Serial.print(p + 1);
  Serial.print(F(" press btn "));
  Serial.println(btn, HEX);
#endif
}

void BtnRelease(byte button) {
  int p = button >> 7;
  int btn = button & 0b111;
  if (pMouse == nullptr)
    return;
  // if the mouse is pressed, release it:
  if (pMouse->isPressed(MouseButtons[btn], p == 1)) {
    pMouse->release(MouseButtons[btn], p == 1);
  }
  ButtonStateHasChanged = true;
#ifdef DEBUG_PRINTF
  Serial.print(F("Mmouse P"));
  Serial.print(p + 1);
  Serial.print(F(" release btn "));
  Serial.println(btn, HEX);
#endif
}

void MoveAxis(byte axis, int8_t value) {
  int p = axis >> 7;
  int8_t signvalue = ((axis & 0b1000) ? -value : value);
  int dir = axis & 0b111;  // W/Y/X bitmask

  int8_t xDistance = (dir == 0b0001 ? signvalue : 0);
  int8_t yDistance = (dir == 0b0010 ? signvalue : 0);
  int8_t wDistance = (dir == 0b0100 ? signvalue : 0);

  if (pMouse == nullptr)
    return;
  pMouse->move(xDistance, yDistance, wDistance, p == 1);
  MoveStateHasChanged = true;
#ifdef DEBUG_PRINTF
  Serial.print(F("Mmouse P"));
  Serial.print(p + 1);
  Serial.print(F(" move x "));
  Serial.print(xDistance);
  Serial.print(F(" y "));
  Serial.print(yDistance);
  Serial.print(F(" w "));
  Serial.println(wDistance);
#endif
}

void UpdateToPC() {
  if (pMouse == nullptr)
    return;
  if (ButtonStateHasChanged || MoveStateHasChanged) {
    pMouse->sendReport(false);
    pMouse->sendReport(true);
    ButtonStateHasChanged = false;
    if (MoveStateHasChanged) {
      // Clear move data
      pMouse->move(0, 0, 0, false);
      pMouse->move(0, 0, 0, true);
      MoveStateHasChanged = false;
    }
  }

#ifdef DEBUG_PRINTF
  //Serial.println(F("mouse update"));
#endif
}

}
#endif
