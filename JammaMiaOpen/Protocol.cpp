#include "Config.h"
/*
  Management of configuration for IO board
*/
#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"
#include "CRC.h"

//#define WAIT_USB_AT_BOOT

namespace Protocol {

void SetupPort() {
  // initialize serial communications at maximum baudrate bps:
  long baudrate = 1000000;
  switch (Config::ConfigFile.SerialSpeed) {
    case Config::COM38400:
      baudrate = 38400;
      break;
    case Config::COM57600:
      baudrate = 57600;
      break;
    case Config::COM115200:
      baudrate = 115200;
      break;
    case Config::COM250000:
      baudrate = 250000;
      break;
    case Config::COM500000:
      baudrate = 500000;
      break;
    case Config::COM1000000:
      baudrate = 1000000;
      break;
  }
  Serial.begin(baudrate);

#ifdef WAIT_USB_AT_BOOT
  // Wait until USB ready or after 2x10ms
  while (!Serial)
    ;
  // Serial Timeout 1000ms
  Serial.setTimeout(1000);
#else
  // Serial Timeout 1000ms
  Serial.setTimeout(1000);
#endif
}

//-----------------------------------------------------------------------------
// Simple commands
//-----------------------------------------------------------------------------

// Footer = (end of frame)
// - End of line '\n' (1 byte)
void SendEOF() {
  // Add '\n' for end-of-frame
  Serial.write('\n');
}

// Send a uint32 value with "n" hexa digits
void SendXWord(uint32_t val, int ndigits) {
  String buff;
  Utils::ConvertToNDigHex(val, buff, ndigits);
  Serial.write(buff.c_str(), ndigits);
}

void SendDebugMessageFrame(const String &debug) {
  if (!Globals::VolatileConfig.DebugMode)
    return;
  Serial.print("M");
  Serial.print(debug);
  SendEOF();
}

void SendDebugKeyText(const String &key, const String &txt) {
  String buffer;
  buffer += key;
  buffer += "=";
  buffer += txt;
  SendDebugMessageFrame(buffer);
}

void SendDebugKeyValue(const String &msg, const String &key, uint32_t value, int ndigits) {
  String buffer;
  buffer += msg;
  buffer += key;
  buffer += "=";
  String zvalue;
  Utils::ConvertToNDigHex(value, zvalue, ndigits);
  buffer += zvalue;
  SendDebugMessageFrame(buffer);
}


void SendStatusFrame() {
  Serial.write('M');
  Serial.print(F("mcp1="));
  Serial.print(Globals::MCPIOs[0], HEX);
  Serial.print(" mcp2=");
  Serial.print(Globals::MCPIOs[1], HEX);
  Serial.print(" mcu=");
  Serial.print(Globals::MCUIOs, HEX);
  Serial.print(" an0=");
  Serial.print(Globals::AIn[0], HEX);
  Serial.print(" an1=");
  Serial.print(Globals::AIn[1], HEX);
  Serial.print(" an2=");
  Serial.print(Globals::AIn[2], HEX);
  Serial.print(" an3=");
  Serial.print(Globals::AIn[3], HEX);

  Serial.print(" ao0=");
  Serial.print(Globals::AOut[0], HEX);
  Serial.print(" ao1=");
  Serial.print(Globals::AOut[1], HEX);
  Serial.print(" ao2=");
  Serial.print(Globals::AOut[2], HEX);
  Serial.print(" ao3=");
  Serial.print(Globals::AOut[3], HEX);

  Serial.print(" rrate=");
  Serial.print(Globals::refreshRate_us);
  Serial.print(" us");
  SendEOF();
}

void SendErrorFrame(int code, const String &msg) {
  Serial.write('S');
  SendXWord(code, 4);
  Serial.write(' ');
  Serial.print(msg);
  SendEOF();
}

void SendMessageFrame(const String &msg) {
  Serial.write('M');
  Serial.print(msg);
  SendEOF();
}

void SendKeyText(const String &key, const String &txt) {
  String buffer;
  buffer += key;
  buffer += "=";
  buffer += txt;
  SendMessageFrame(buffer);
}

void SendKeyValuepair(const String &msg, const String &key, uint32_t value, int ndigits) {
  
  String buffer;
  buffer += msg;
  buffer += key;
  buffer += "=";
  String zvalue;
  Utils::ConvertToNDigHex(value, zvalue, ndigits);
  buffer += zvalue;
  SendMessageFrame(buffer);
}


//-----------------------------------------------------------------------------
// Complex commands: parameters and commands
//-----------------------------------------------------------------------------

// Accessible parameter's types
enum Types : byte {
  BYTE = 0,
  UINT8,
  INT8,
  UINT16,
  INT16,
  FLOAT
};

// Parameter entry: key/type/reference
typedef struct
{
  const char *Key;
  enum Types Type;
  void *pValue;
} DictionaryParamEntry;

// Parameter list
static const DictionaryParamEntry DictionaryParam[] = {
  { "serialspeed", BYTE, (void *)&Config::ConfigFile.SerialSpeed },
  { "delay", UINT16, (void *)&Config::ConfigFile.Delay_us },
  { "klay", BYTE, (void *)&Config::ConfigFile.KeybLayout },
  { "emode", BYTE, (void *)&Config::ConfigFile.EmulationMode },
};

// command handlers
void ResetCfgHandler(const String &keyval);
void LoadCfgHandler(const String &keyval);
void SaveCfgHandler(const String &keyval);
void GetHandler(const String &key);
void SetHandler(const String &key);
void HelpHandler(const String &key);



// Command entry: key/handler
typedef struct
{
  const char *Keyword;
  void (*pHandler)(const String &key);
} DictionaryKeywordEntry;

// Command list
static const DictionaryKeywordEntry DictionaryKeyword[] = {
  { "resetcfg", ResetCfgHandler },  // Reset configuration
  { "loadcfg", LoadCfgHandler },    // Load configuration from eprom
  { "savecfg", SaveCfgHandler },    // Save configuration in eprom
  { "get", GetHandler },            // Get parameter
  { "set", SetHandler },            // Set parameter
  { "help", HelpHandler },          // Help
};



// Handler for "Get parameter" command
void GetHandler(const String &key) {
  //SendDebugKeyText("get key", key);

  int count = sizeof(DictionaryParam) / sizeof(DictionaryParam[0]);
  int i;
  for (i = 0; i < count; i++) {
    if (key.equals(DictionaryParam[i].Key)) {
      switch (DictionaryParam[i].Type) {
        case BYTE:
        case INT8:
        case UINT8:
          {
            uint8_t value = *((uint8_t *)DictionaryParam[i].pValue);
            SendKeyValuepair("", key, value, 2);
          }
          break;
        case INT16:
        case UINT16:
          {
            uint16_t value = *((uint16_t *)DictionaryParam[i].pValue);
            SendKeyValuepair("", key, value, 4);
          }
          break;
        case FLOAT:
          {
            uint32_t value = *((uint32_t *)DictionaryParam[i].pValue);
            SendKeyValuepair("", key, value, 8);
          }
          break;
        default:
          {
            SendErrorFrame(3, "Unknown type " + String(DictionaryParam[i].Type) + " for param " + key);
          }
          break;
      }
      break;
    }
  }
  if (i == count) {
    SendErrorFrame(3, "Key " + key + " not found");
  }
}

// Handler for "Set parameter" command
// Always use hex numbers, whatever is the underlying value.
// The reply message will contain the type and real value that has been set.
// Example:
// $set val=FFFFFFFF
// Set float val=1.0f
void SetHandler(const String &keyval) {
  String key = Utils::Token(keyval, '=', 0);
  // Skip space
  key.trim();
  String value = Utils::Token(keyval, '=', 1);
  value.trim();
  //SendDebugKeyText(key, value);

  int count = sizeof(DictionaryParam) / sizeof(DictionaryParam[0]);
  int i;
  for (i = 0; i < count; i++) {
    if (key.equals(DictionaryParam[i].Key)) {
      switch (DictionaryParam[i].Type) {
        case BYTE:
          {
            byte val = (byte)Utils::ConvertHexToInt(value, 8);
            *((byte *)DictionaryParam[i].pValue) = val;
            //SendKeyValuepair("Set byte ", key, val, 2);
          }
          break;
        case UINT8:
          {
            uint8_t val = (uint8_t)Utils::ConvertHexToInt(value, 8);
            *((uint8_t *)DictionaryParam[i].pValue) = val;
            //SendKeyValuepair("Set uint8 ", key, val, 2);
          }
          break;
        case INT8:
          {
            int8_t val = (int8_t)Utils::ConvertHexToInt(value, 8);
            *((int8_t *)DictionaryParam[i].pValue) = val;
            //SendKeyValuepair("Set int8 ", key, val, 2);
          }
          break;
        case UINT16:
          {
            uint16_t val = (uint16_t)Utils::ConvertHexToInt(value, 8);
            *((uint16_t *)DictionaryParam[i].pValue) = val;
            //SendKeyValuepair("Set uint16 ", key, val, 4);
          }
          break;
        case INT16:
          {
            int16_t val = (int16_t)Utils::ConvertHexToInt(value, 8);
            *((int16_t *)DictionaryParam[i].pValue) = val;
            //SendKeyValuepair("Set int16 ", key, val, 4);
          }
          break;
        case FLOAT:
          {
            uint32_t val = (uint32_t)Utils::ConvertHexToInt(value, 8);
            *((uint32_t *)DictionaryParam[i].pValue) = val;
            //float *pval = (float *)DictionaryParam[i].pValue;
            //SendKeyValuepair("Set float as uint32 ", key, val, 8);
          }
          break;
        default:
          {
            SendErrorFrame(3, (String)F("Unknown type ") + String(DictionaryParam[i].Type) + (String)F(" for param ") + key);
          }
          break;
      }
      break;
    }
  }
  if (i == count) {
    SendErrorFrame(3, "Key " + key + " not found");
  }
}

// Handler for "Reset configuration" command
void ResetCfgHandler(__attribute__((unused)) const String &keyval) {
  Config::ResetConfig();
  Config::SaveConfigToEEPROM();
  SendMessageFrame(F("EEPROM reset and write"));
}

// Handler for "Load configuration from eprom" command
void LoadCfgHandler(__attribute__((unused)) const String &keyval) {
  int stt = Config::LoadConfigFromEEPROM();
  if (stt == 1)
    SendMessageFrame(F("EEPROM load"));
  else {
    //SendKeyValuepair(F("Error load EEPROM failed with "), "stt", stt, 4);
  }
}

// Handler for "Save configuration to eprom" command
void SaveCfgHandler(__attribute__((unused)) const String &keyval) {
  int stt = Config::SaveConfigToEEPROM();
  if (stt == 1)
    SendMessageFrame(F("EEPROM save"));
  else {
    //SendKeyValuepair(F("Error save EEPROM failed with "), "stt", stt, 4);
  }
}

// Handler for "Help" command
void HelpHandler(__attribute__((unused)) const String &keyval) {
  int i;
  int countkwd = sizeof(DictionaryKeyword) / sizeof(DictionaryKeyword[0]);
  for (i = 0; i < countkwd; i++) {
    SendKeyText(F("Keyword "), DictionaryKeyword[i].Keyword);
  }
  int countparam = sizeof(DictionaryParam) / sizeof(DictionaryParam[0]);
  for (i = 0; i < countparam; i++) {
    //SendKeyValuepair(F("Param "), DictionaryParam[i].Key, DictionaryParam[i].Type, 2);
  }
  SendMessageFrame((String)F("Help listed ") + String(countkwd) + F(" keywords and ") + String(countparam) + F(" params"));
}


//-----------------------------------------------------------------------------
// Interpreter
//-----------------------------------------------------------------------------


// Cresetcfg: reset config and save
// Cloadcfg: load config from eeprom
// Csavecfg: write eeprom
// Cget/set fmin1=XX: set filter 1 min freq
// Cget/set serial=XX: set serial speed (0..4)
void InterpretCommand(char *pline) {
  String line = String(pline);
  // Skip spaces
  //line.trim();
  // get first token after space
  String command = Utils::Token(line, ' ', 0);
  //command.trim();
  int count = sizeof(DictionaryKeyword) / sizeof(DictionaryKeyword[0]);
  int i;
  for (i = 0; i < count; i++) {
    if (command.equals(DictionaryKeyword[i].Keyword)) {
      String remaining = line.substring(command.length());
      // skip leading empty space
      remaining.trim();
      DictionaryKeyword[i].pHandler(remaining);
      break;
    }
  }
  if (i == count) {
    SendErrorFrame(3, F("Syntax error"));
  }
}


//-----------------------------------------------------------------------------
// Message processing
//-----------------------------------------------------------------------------


int ProcessOneMessage() {
  if (Serial.available() > 0) {
    char msg[96 + 16];

    size_t read = Serial.readBytesUntil('\n', msg, sizeof(msg));
    if (read > 0) {
      // Enforce null-terminated string (remove '\n')
      msg[read] = 0;

      size_t index = 0;

      while (index < read) {

        switch (msg[index++]) {
          case '~':
            {
              // Reset Board
              Utils::SoftwareReboot();
            }
            break;
          case '$':
            {
              // Command line, until newline
              InterpretCommand(&msg[0] + index);
              index = read;
            }
            break;

          case 'D':
            {
              Globals::VolatileConfig.DebugMode = true;
              SendDebugMessageFrame(F("Debug ON"));
            }
            break;
          case 'd':
            {
              Globals::VolatileConfig.DebugMode = false;
              SendDebugMessageFrame(F("Debug OFF"));
            }
            break;
          case '?':
            {
              // Handshaking!
              // Read protocol version
              /*
              char *sc = (char *)(msg + index);
              int major = Utils::ConvertHexToInt(sc, 4);
              int minor = Utils::ConvertHexToInt(sc + 4, 4);
              (String)F("recv major=") + String(major, HEX) + (String)F(" minor=") + String(minor, HEX)
              SendDebugMessageFrame();
              if (major >= PROTOCOL_VERSION_MAJOR) {
                SendDebugMessageFrame(F("handshaking ok"));
              }
              */
              // Send protocol version - hardcoded
              Serial.print(F("?"));
              SendXWord(PROTOCOL_VERSION_MAJOR, 4);
              SendXWord(PROTOCOL_VERSION_MINOR, 4);
              SendEOF();
              // frame terminated
              index = read;
            }
            break;

          case 'V':
            {
              // Board version - hardcoded
              Serial.print(F(VERSION_STRING));
              SendEOF();
              index = read;
            }
            break;

          case 'U':
            {
              // Send single status frame
              if (!Globals::VolatileConfig.DoStreaming) {
                SendStatusFrame();
              }
            }
            break;

          case 'S':
            {
              // Start streaming
              Globals::VolatileConfig.DoStreaming = true;
              index = read;
            }
            break;

          case 'H':
            {
              // Halt streaming
              Globals::VolatileConfig.DoStreaming = false;
              index = read;
            }
            break;

          case 'O':
            {
              // Set digital outputs value 4..7 (only 4) : OXX with XX being a value between 0..F that enable/disable an output
              char *sc = (char *)(msg + index);
              int do_value = Utils::ConvertHexToInt(sc, 2);
              for (int i = 0; i < NB_DIGITALOUTPUTS; i++) {
                Globals::DOut[i] = (do_value >> i) & 1;
              }
              SendDebugMessageFrame("O=" + String(do_value, HEX));
              index += 2;
            }
            break;

          case 'P':
            {
              // pwm block analog out (4x) : PXYY with X being a 4-bit selector and YY being a value between 0..FF
              char *sc = (char *)(msg + index);
              int do_value = Utils::ConvertHexToInt(sc, 3);
              for (int i = 0; i < NB_ANALOGOUTPUTS; i++) {
                bool selected = (do_value >> (i + 8)) & 1;
                if (selected) {
                  Globals::AOut[i] = do_value & 0xFF;
                }
              }
              SendDebugMessageFrame("pwm=" + String(do_value, HEX));
              index += 3;
            }
            break;

          default:
            SendErrorFrame(0, (String)F("UNKNOWN CMD ") + String(msg));
            index = read;
            break;
        }
      }


      return 1;
    }
  }
  return 0;
}

}
