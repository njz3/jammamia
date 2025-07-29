/*
  Management of configuration for IO board
*/
#include "Config.h"
#include "Protocol.h"
#include "Globals.h"
#include "Utils.h"
#include "CRC.h"

//#define WAIT_USB_AT_BOOT

namespace Protocol {

void SetupPort() {
  // initialize serial communications at maximum baudrate bps:
  Serial.begin(PCSERIAL_BAUDRATE);

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

// Send a uint32 value with "n" hexa digits
void SendXWord(uint32_t val, int ndigits) {
  String buff;
  Utils::ConvertToNDigHex(val, buff, ndigits);
  Serial.write(buff.c_str(), ndigits);
}

const char PROGMEM sSPC[] = " ";
const char PROGMEM sE01[] = "E01 Unknown keyw ";
const char PROGMEM sE02[] = "E02 Key not found ";
const char PROGMEM sE03[] = "E03 Unknown type for ";

void SendStatusFrame() {
  Serial.write('S');
  Serial.print(F("mcp1="));
  Serial.print(Globals::MCPIOs[0], HEX);
  Serial.print(F(" mcp2="));
  Serial.print(Globals::MCPIOs[1], HEX);
  Serial.print(F(" mcu="));
  Serial.print(Globals::MCUIOs, HEX);
  Serial.print(F(" an="));
  for (int i = 0; i < NB_ANALOGINPUTS; i++) {
    Serial.print(Globals::AIn[i], HEX);
    Serial.print((__FlashStringHelper *)sSPC);
  }
  Serial.print(F("do="));
  for (int i = 0; i < NB_DIGITALOUTPUTS; i++) {
    Serial.print(Globals::DOut[i], HEX);
    Serial.print((__FlashStringHelper *)sSPC);
  }
  Serial.print(F("ao="));
  for (int i = 0; i < NB_ANALOGOUTPUTS; i++) {
    Serial.print(Globals::AOut[i], HEX);
    Serial.print((__FlashStringHelper *)sSPC);
  }

  Serial.print(F("rr_us="));
  Serial.println(Globals::refreshRate_us);
}

void SendMessageFrame(const String &msg) {
  Serial.write('M');
  Serial.println(msg);
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
  buffer += "=0x";
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
  UINT8 = 0,
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
  { "delay", UINT16, (void *)&Config::ConfigFile.Delay_us },
  { "kblay", UINT8, (void *)&Config::ConfigFile.KeybLayout },
  { "emode", UINT8, (void *)&Config::ConfigFile.EmulationMode },
  { "axes", UINT8, (void *)&Config::ConfigFile.JoyNumberOfAxes },
  { "btns", UINT8, (void *)&Config::ConfigFile.JoyNumberOfButtons },
  { "hats", UINT8, (void *)&Config::ConfigFile.JoyNumberOfHAT },
  { "shift", UINT8, (void *)&Config::ConfigFile.ShiftInput },
};

// command handlers
void ResetCfgHandler(const String &keyval);
void LoadCfgHandler(const String &keyval);
void SaveCfgHandler(const String &keyval);
void GetHandler(const String &key);
void SetHandler(const String &key);
void HelpHandler(const String &key);
void SetDInMapHandler(const String &key);
void SetAInMapHandler(const String &key);



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
  { "setdin", SetDInMapHandler },   // Set mapping for digital input
  { "setain", SetAInMapHandler },   // Set mapping for analog input
};

// Handler for "Get parameter" command
void GetHandler(const String &key) {
  //SendDebugKeyText("get key", key);

  int count = sizeof(DictionaryParam) / sizeof(DictionaryParam[0]);
  int i;
  for (i = 0; i < count; i++) {
    if (key.equals(DictionaryParam[i].Key)) {
      switch (DictionaryParam[i].Type) {
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
            Serial.print((__FlashStringHelper *)sE03);
            Serial.println(key);
          }
          break;
      }
      break;
    }
  }
  if (i == count) {
    Serial.print((__FlashStringHelper *)sE02);
    Serial.println(key);
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
  String value = Utils::Token(keyval, '=', 1);

  int count = sizeof(DictionaryParam) / sizeof(DictionaryParam[0]);
  int i;
  for (i = 0; i < count; i++) {
    if (key.equals(DictionaryParam[i].Key)) {
      switch (DictionaryParam[i].Type) {
        case UINT8:
          {
            uint8_t val = (uint8_t)Utils::ConvertHexToInt(value, 8);
            *((uint8_t *)DictionaryParam[i].pValue) = val;
          }
          break;
        case INT8:
          {
            int8_t val = (int8_t)Utils::ConvertHexToInt(value, 8);
            *((int8_t *)DictionaryParam[i].pValue) = val;
          }
          break;
        case UINT16:
          {
            uint16_t val = (uint16_t)Utils::ConvertHexToInt(value, 8);
            *((uint16_t *)DictionaryParam[i].pValue) = val;
          }
          break;
        case INT16:
          {
            int16_t val = (int16_t)Utils::ConvertHexToInt(value, 8);
            *((int16_t *)DictionaryParam[i].pValue) = val;
          }
          break;
        default:
          {
            Serial.print((__FlashStringHelper *)sE03);
            Serial.println(key);
          }
          break;
      }
      break;
    }
  }
  if (i == count) {
    Serial.print((__FlashStringHelper *)sE02);
    Serial.println(key);
  }
}

// Handler for "Reset configuration" command
void ResetCfgHandler(__attribute__((unused)) const String &keyval) {
  Config::ResetConfig();
  Serial.println(F("Mcfg rst"));
}

// Handler for "Load configuration from eprom" command
void LoadCfgHandler(__attribute__((unused)) const String &keyval) {
  int stt = Config::LoadConfigFromEEPROM();
  if (stt == 1)
    Serial.println(F("Mcfg ld"));
  else {
    //SendKeyValuepair(F("Error load EEPROM failed with "), "stt", stt, 4);
  }
}

// Handler for "Save configuration to eprom" command
void SaveCfgHandler(__attribute__((unused)) const String &keyval) {
  int stt = Config::SaveConfigToEEPROM();
  if (stt == 1)
    Serial.println(F("Mcfg svd"));
  else {
    //SendKeyValuepair(F("Error save EEPROM failed with "), "stt", stt, 4);
  }
}

// Handler for "Help" command
void HelpHandler(__attribute__((unused)) const String &keyval) {
  int i;
  int countkwd = sizeof(DictionaryKeyword) / sizeof(DictionaryKeyword[0]);
  int countparam = sizeof(DictionaryParam) / sizeof(DictionaryParam[0]);
  Serial.println((String)F("MHelp ") + String(countkwd) + F(" ") + String(countparam));
  for (i = 0; i < countkwd; i++) {
    Serial.println((String)F("MKwd ") + DictionaryKeyword[i].Keyword);
  }
  for (i = 0; i < countparam; i++) {
    SendKeyValuepair(F("Par "), DictionaryParam[i].Key, DictionaryParam[i].Type, 2);
  }
}

// setdin DIN TYPE MAP SHIFTEDMAP NAME
// DIN: digital input number
// TYPE: type value
// MAP: map value
// SHIFTEDMAP: shifted map value (0 for none)
// NAME: Name of input (limited to 3 char)
void SetDInMapHandler(const String &keyval) {
  String token = Utils::Token(keyval, ' ', 0);
  uint8_t din = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 1);
  uint8_t type = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 2);
  uint8_t mapp = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 3);
  uint8_t shiftedmap = (uint32_t)Utils::ConvertHexToInt(token, 2);
  Config::ConfigFile.DigitalInB[din].Type = (Config::MappingType)type;
  Config::ConfigFile.DigitalInB[din].MapTo = mapp;
  Config::ConfigFile.DigitalInB[din].MapToShifted = shiftedmap;
  token = Utils::Token(keyval, ' ', 4);
  const char *c_name = token.c_str();
  strncpy(Config::ConfigFile.DigitalInB[din].Name, c_name, 3);
  Config::PrintDInConfig(din);
}

// setain AIN TYPE POS NEG DMIN DMAX NAME
// AIN: analog input axes number
// TYPE: type value
// POS: map value when going positive
// NEG: map value when going negative
// DMIN: dead zone min value if hat or button, usually 60
// DMAX: dead zone max value if hat or button, usually 80
// NAME: Name of analog input (limited to 3 char)
void SetAInMapHandler(const String &keyval) {
  String token = Utils::Token(keyval, ' ', 0);
  uint8_t ain = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 1);
  uint8_t type = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 2);
  uint8_t pos = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 3);
  uint8_t neg = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 4);
  uint8_t dmin = (uint32_t)Utils::ConvertHexToInt(token, 2);
  token = Utils::Token(keyval, ' ', 5);
  uint8_t dmax = (uint32_t)Utils::ConvertHexToInt(token, 2);
  Config::ConfigFile.AnalogInDB[ain].Type = (Config::MappingType)type;
  Config::ConfigFile.AnalogInDB[ain].MapToPos = pos;
  Config::ConfigFile.AnalogInDB[ain].MapToNeg = neg;
  Config::ConfigFile.AnalogInDB[ain].DeadzoneMin = dmin;
  Config::ConfigFile.AnalogInDB[ain].DeadzoneMax = dmax;
  token = Utils::Token(keyval, ' ', 6);
  const char *c_name = token.c_str();
  strncpy(Config::ConfigFile.AnalogInDB[ain].Name, c_name, 3);
  Config::PrintAInConfig(ain);
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
    Serial.println((String)sE03);
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

          case 'd':
            {
              Globals::VolatileConfig.DebugMode = true;
              Serial.println(F("MDebug ON"));
            }
            break;
          case 'D':
            {
              Globals::VolatileConfig.DebugMode = false;
              Serial.println(F("MDebug OFF"));
            }
            break;
          case '?':
            {
              // Handshaking!
              // Send protocol version - hardcoded
              Serial.println(F("?" PROTOCOL_VERSION_MAJOR PROTOCOL_VERSION_MINOR));
              // frame terminated
              index = read;
            }
            break;

          case 'v':
            {
              // Board version - hardcoded
              Serial.println(F(VERSION_STRING));
              index = read;
            }
            break;

          case 'u':
            {
              // Send single status frame
              if (!Globals::VolatileConfig.DoStreaming) {
                SendStatusFrame();
              }
            }
            break;

          case 's':
            {
              // Start streaming
              Globals::VolatileConfig.DoStreaming = true;
              index = read;
            }
            break;
          case 'e':
            {
              // Start streaming
              Globals::VolatileConfig.DoEmulation = true;
              index = read;
            }
            break;
          case 'E':
            {
              // Start streaming
              Globals::VolatileConfig.DoEmulation = false;
              index = read;
            }
            break;

          case 'h':
            {
              // Halt streaming
              Globals::VolatileConfig.DoStreaming = false;
              index = read;
            }
            break;

          case 'o':
            {
              // Set digital outputs value 0..F (only 4 bits) : oXX with XX being a value between 0..F that enable/disable an output
              char *sc = (char *)(msg + index);
              int do_value = Utils::ConvertHexToInt(sc, 2);
              for (int i = 0; i < NB_DIGITALOUTPUTS; i++) {
                Globals::DOut[i] = (do_value >> i) & 1;
              }
              Serial.println("MO=" + String(do_value, HEX));
              index += 2;
            }
            break;

          case 'p':
            {
              // pwm block analog out (4x) : pXYY with X being a 4-bit selector and YY being a value between 0..FF
              char *sc = (char *)(msg + index);
              int do_value = Utils::ConvertHexToInt(sc, 3);
              for (int i = 0; i < NB_ANALOGOUTPUTS; i++) {
                bool selected = (do_value >> (i + 8)) & 1;
                if (selected) {
                  Globals::AOut[i] = do_value & 0xFF;
                }
              }
              Serial.println("Mpwm=" + String(do_value, HEX));
              index += 3;
            }
            break;

          case 'l':
            Config::PrintConfig();
            index = read;
            break;

          default:
            Serial.print((__FlashStringHelper *)sE01);
            Serial.println(msg);
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
