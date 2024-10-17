/*
  1 byte text protocol
*/
#pragma once
#include "Config.h"

namespace Protocol {

void SetupPort();
void SendDebugMessageFrame(const String &debug);
void SendDebugKeyText(const String &key, const String &txt);
void SendDebugKeyValue(const String &msg, const String &key, uint32_t value, int ndigits);

void SendStatusFrame();
void SendErrorFrame(int code, const String &msg);
void SendMessageFrame(const String &msg);
void SendKeyText(const String &key, const String &txt);
void SendKeyValuepair(const String &msg, const String &key, uint32_t value, int ndigits);

int ProcessOneMessage();

}
