/*
  Management of configuration for IO board
*/
#include "Config.h"
#include <EEPROM.h>
#include "CRC.h"
#include "Globals.h"

#ifdef USE_KEYB
#include <KeyboardNKey.h>
#endif

namespace Config {

EEPROM_CONFIG ConfigFile;

const int EEPROM_CONFIG_START = 0x80;
const int EEPROM_CONFIG_SIZE = sizeof(EEPROM_CONFIG);
const int EEPROM_CONFIG_END = EEPROM_CONFIG_START + EEPROM_CONFIG_SIZE;

const int EEPROM_TOTALSIZE = EEPROM_CONFIG_END;

int SaveConfigToEEPROM() {
  if (EEPROM.length() < EEPROM_TOTALSIZE) {
    return -1;
  }
  // Pointer to record
  byte* pBlock = (byte*)&ConfigFile;
  // Compute CRC8 to detect wrong eeprom data
  byte crc8 = CRC::crc8(pBlock + 1, EEPROM_CONFIG_SIZE - 1);
  // Update CRC in record
  ConfigFile.CRC8 = crc8;
  // Write record to EEPROM
  for (int i = 0; i < EEPROM_CONFIG_SIZE; i++) {
    EEPROM.write(EEPROM_CONFIG_START + i, pBlock[i]);
  }
  return 1;
}

int LoadConfigFromEEPROM() {
  if (EEPROM.length() < EEPROM_TOTALSIZE) {
    return -1;
  }
  // Pointer to a new record on MCU stack
  EEPROM_CONFIG newCfg;
  byte* pBlock = (byte*)&newCfg;
  // Read new record from EEPROM
  for (int i = 0; i < EEPROM_CONFIG_SIZE; i++) {
    pBlock[i] = EEPROM.read(EEPROM_CONFIG_START + i);
  }
  // Compute CRC8 to detect wrong eeprom data
  byte crc8 = CRC::crc8(pBlock + 1, EEPROM_CONFIG_SIZE - 1);
  // Check CRC match?
  if (crc8 != newCfg.CRC8) {
    // Wrong CRC
    return -2;
  }
  // Ok, store new config
  ConfigFile = newCfg;
  return 1;
}

const char PROGMEM sSPC[] = " ";

// din DIN TYPE MAP SHIFTEDMAP NAME
// DIN: digital input number
// TYPE: type value
// MAP: map value
// SHIFTEDMAP: shifted map value (0 for none)
// NAME: Name of input (limited to 3 char)
void PrintDInConfig(int i) {
  Serial.print(F("Mdin "));
  Serial.print(i, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.DigitalInB[i].Type, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.DigitalInB[i].MapTo, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.DigitalInB[i].MapToShifted, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.println(ConfigFile.DigitalInB[i].Name);
}
// ain AIN TYPE POS NEG DMIN DMAX NAME
// AIN: analog input axes number
// TYPE: type value
// POS: map value when going positive
// NEG: map value when going negative
// DMIN: dead zone min value if hat or button, usually 60
// DMAX: dead zone max value if hat or button, usually 80
// NAME: Name of analog input (limited to 3 char)
void PrintAInConfig(int i) {
  Serial.print(F("Main "));
  Serial.print(i, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.AnalogInDB[i].Type, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.AnalogInDB[i].MapToPos, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.AnalogInDB[i].MapToNeg, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.AnalogInDB[i].DeadzoneMin, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.print(ConfigFile.AnalogInDB[i].DeadzoneMax, HEX);
  Serial.print((__FlashStringHelper*)sSPC);
  Serial.println(ConfigFile.DigitalInB[i].Name);
}

void PrintConfig() {
#ifdef DEBUG_PRINTF
  Serial.println(F("MButtons config: type 0=none, 1=keyb, 2=joy axis, 3=joy HAT, 4=joy btn, 5=mouse axes, 6=mouse btn."));
  Serial.println(F("MList of configured digital inputs (0x8X means player 2):"));
#endif
  for (uint8_t i = 0; i < sizeof(ConfigFile.DigitalInB) / sizeof(ConfigFile.DigitalInB[0]); i++) {
    PrintDInConfig(i);
  }
  for (uint8_t i = 0; i < sizeof(ConfigFile.AnalogInDB) / sizeof(ConfigFile.AnalogInDB[0]); i++) {
    PrintAInConfig(i);
  }
}
// Offset to get P2 digital inputs
#define P2_DIN_OFFSET (14)

// Reset to default values
void ResetConfig() {
  memset(&ConfigFile, 0, sizeof(ConfigFile));
  ConfigFile.Delay_us = 0;
  // Emulated layout
  ConfigFile.KeybLayout = 0;  // Layout en-US
  //ConfigFile.KeybLayout = 1;  // Layout fr-FR

#if defined(USE_JOY) && !defined(USE_KEYB) && !defined(USE_MOUSE)
  // Joystick only
  Config::ConfigFile.EmulationMode = Config::EmulationModes::Joystick;
  ConfigFile.ShiftInput = 0;           // No Shift input
  ConfigFile.JoyNumberOfButtons = 12;  // 8+2 COIN/START + 2 cabinet buttons per player (TEST/SERV for P1 and TEST2/TILT for P2)
  ConfigFile.JoyNumberOfAxes = 2;      // 2 axes per joystick
  ConfigFile.JoyNumberOfHAT = 1;       // 1 HAT per joystick
  // 8x Buttons
  for (uint8_t i = 0; i < 8; i++) {
    // P1
    ConfigFile.DigitalInB[i].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i].MapTo = (byte)i;
    // P2
    ConfigFile.DigitalInB[i + P2_DIN_OFFSET].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + P2_DIN_OFFSET].MapTo = (byte)i + (byte)(1 << 7);
  }
  // 4x HAT directions
  for (uint8_t i = 0; i < 4; i++) {
    // P1
    ConfigFile.DigitalInB[i + 8].Type = MappingType::JoyDirHAT;
    ConfigFile.DigitalInB[i + 8].MapTo = (byte)(1 << i);
    // P2
    ConfigFile.DigitalInB[i + 8 + P2_DIN_OFFSET].Type = MappingType::JoyDirHAT;
    ConfigFile.DigitalInB[i + 8 + P2_DIN_OFFSET].MapTo = (byte)(1 << i) + (byte)(1 << 7);
  }
  // 2x Coin/Start
  for (uint8_t i = 0; i < 2; i++) {
    // P1
    ConfigFile.DigitalInB[i + 12].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 12].MapTo = (byte)(i + 8);
    // P2
    ConfigFile.DigitalInB[i + 12 + P2_DIN_OFFSET].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 12 + P2_DIN_OFFSET].MapTo = (byte)(i + 8) + (byte)(1 << 7);
  }

  // Map Service buttons that are on MCU pins
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+0].Type = MappingType::JoyButton;  // TEST
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+0].MapTo = 10;                     // TEST
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+1].Type = MappingType::JoyButton;  // SERVICE
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+1].MapTo = 11;                     // SERVICE
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+2].Type = MappingType::JoyButton;  // TEST2
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+2].MapTo = (byte)(1 << 7) + 10;    // TEST2
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+3].Type = MappingType::JoyButton;  // TILT
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+3].MapTo = (byte)(1 << 7) + 11;    // TILT

  // 4x analog sticks on analog inputs screw terminals
  for (uint8_t i = 0; i < sizeof(ConfigFile.AnalogInDB) / sizeof(ConfigFile.AnalogInDB[0]); i++) {
    ConfigFile.AnalogInDB[i].Type = MappingType::JoyAxis;
    ConfigFile.AnalogInDB[i].MapToPos = i % 2 + ((i < 2) ? 0 : (byte)(1 << 7));  // X/Y/Z
    ConfigFile.AnalogInDB[i].MapToNeg = 0;                                       // X/Y/Z

    ConfigFile.AnalogInDB[i].DeadzoneMin = 0x60;  // dead zone min (x4) to detect axis leave center zone
    ConfigFile.AnalogInDB[i].DeadzoneMax = 0xA0;  // dead zone max (x4) to detect axis leave for center zone
  }
#endif

#if defined(USE_KEYB) && !defined(USE_JOY) && !defined(USE_MOUSE)
  // Keyboard only
  Config::ConfigFile.EmulationMode = Config::EmulationModes::Keyboard;
  ConfigFile.ShiftInput = 14;  // Shift input "P1-START" din index is 13+1 since index is 1-based

  // Emulated keys for all digital inputs
  for (uint8_t i = 0; i < sizeof(ConfigFile.DigitalInB) / sizeof(ConfigFile.DigitalInB[0]); i++) {
    ConfigFile.DigitalInB[i].Type = MappingType::Key;
    ConfigFile.DigitalInB[i].MapToShifted = 0;  // default to no alternative mapping
  }

  // Emulated keys, using MAME default layout
  // Map Player1 on MCP1
  ConfigFile.DigitalInB[0].MapTo = KEY_LEFT_CTRL;       // P1-But1
  ConfigFile.DigitalInB[0].MapToShifted = '5';          // P1-But1 shifted
  ConfigFile.DigitalInB[1].MapTo = KEY_LEFT_ALT;        // P1-But2
  ConfigFile.DigitalInB[2].MapTo = ' ';                 // P1-But3
  ConfigFile.DigitalInB[3].MapTo = KEY_LEFT_SHIFT;      // P1-But4
  ConfigFile.DigitalInB[4].MapTo = 'z';                 // P1-But5
  ConfigFile.DigitalInB[5].MapTo = 'x';                 // P1-But6
  ConfigFile.DigitalInB[6].MapTo = 'c';                 // P1-But7
  ConfigFile.DigitalInB[7].MapTo = 'v';                 // P1-But8
  
  ConfigFile.DigitalInB[8].MapTo = KEY_UP_ARROW;        // P1-Up
  ConfigFile.DigitalInB[8].MapToShifted = '~';          // P1-Up shifted
  ConfigFile.DigitalInB[9].MapTo = KEY_DOWN_ARROW;      // P1-Down
  ConfigFile.DigitalInB[9].MapToShifted = 'p';          // P1-Down shifted
  ConfigFile.DigitalInB[10].MapTo = KEY_LEFT_ARROW;     // P1-Left
  ConfigFile.DigitalInB[10].MapToShifted = KEY_RETURN;  // P1-Left shifted
  ConfigFile.DigitalInB[11].MapTo = KEY_RIGHT_ARROW;    // P1-Right
  ConfigFile.DigitalInB[11].MapToShifted = KEY_TAB;     // P1-Right shifted
  ConfigFile.DigitalInB[12].MapTo = '5';                // P1-COIN1
  ConfigFile.DigitalInB[13].MapTo = '1';                // P1-START1

  // Map Player2 on MCP2
  ConfigFile.DigitalInB[P2_DIN_OFFSET+0].MapTo = 'a';  // P2-But1
  ConfigFile.DigitalInB[P2_DIN_OFFSET+1].MapTo = 's';  // P2-But2
  ConfigFile.DigitalInB[P2_DIN_OFFSET+2].MapTo = 'q';  // P2-But3
  ConfigFile.DigitalInB[P2_DIN_OFFSET+3].MapTo = 'w';  // P2-But4
  ConfigFile.DigitalInB[P2_DIN_OFFSET+4].MapTo = 'i';  // P2-But5
  ConfigFile.DigitalInB[P2_DIN_OFFSET+5].MapTo = 'k';  // P2-But6
  ConfigFile.DigitalInB[P2_DIN_OFFSET+6].MapTo = 'j';  // P2-But7
  ConfigFile.DigitalInB[P2_DIN_OFFSET+7].MapTo = 'l';  // P2-But8

  ConfigFile.DigitalInB[P2_DIN_OFFSET+8].MapTo = 'r';             // P2-Up
  ConfigFile.DigitalInB[P2_DIN_OFFSET+9].MapTo = 'f';             // P2-Down
  ConfigFile.DigitalInB[P2_DIN_OFFSET+10].MapTo = 'd';             // P2-Left
  ConfigFile.DigitalInB[P2_DIN_OFFSET+11].MapTo = 'g';             // P2-Right
  ConfigFile.DigitalInB[P2_DIN_OFFSET+12].MapTo = '6';             // P2-COIN2
  ConfigFile.DigitalInB[P2_DIN_OFFSET+13].MapTo = '2';             // P2-START2
  ConfigFile.DigitalInB[P2_DIN_OFFSET+13].MapToShifted = KEY_ESC;  // P2-START2 shifted

  // Map Service buttons that are on MCU pins
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+0].MapTo = KEY_F2;  // 'F2' for TEST
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+1].MapTo = KEY_F1;  // 'F1' for SERVICE
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+2].MapTo = 't';     // 't' for TEST2
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+3].MapTo = KEY_F3;  // 'F3' for TILT

  // 4x analog sticks on analog inputs screw terminals mapped to KEYPAD
  for (uint8_t i = 0; i < sizeof(ConfigFile.AnalogInDB) / sizeof(ConfigFile.AnalogInDB[0]); i++) {
    ConfigFile.AnalogInDB[i].Type = MappingType::Key;
    ConfigFile.AnalogInDB[i].DeadzoneMin = 0x60;  // dead zone min (x4) to detect axis leave center zone
    ConfigFile.AnalogInDB[i].DeadzoneMax = 0xA0;  // dead zone max (x4) to detect axis leave for center zone
  }
  /*
  ConfigFile.AnalogInDB[0].MapToPos = KEY_KP_8;
  ConfigFile.AnalogInDB[0].MapToNeg = KEY_KP_2;
  ConfigFile.AnalogInDB[1].MapToPos = KEY_KP_6;
  ConfigFile.AnalogInDB[1].MapToNeg = KEY_KP_4;
  ConfigFile.AnalogInDB[2].MapToPos = KEY_KP_9;
  ConfigFile.AnalogInDB[2].MapToNeg = KEY_KP_1;
  ConfigFile.AnalogInDB[3].MapToPos = KEY_KP_7;
  ConfigFile.AnalogInDB[3].MapToNeg = KEY_KP_3;
  */
#endif


#if defined(USE_JOY) && defined(USE_KEYB) && !defined(USE_MOUSE)
  // Keyboard and Joystick
  Config::ConfigFile.EmulationMode = Config::EmulationModes::JoystickAndKeyboard;
  ConfigFile.ShiftInput = 0;           // No Shift input
  ConfigFile.JoyNumberOfButtons = 10;  // 8+2 COIN/START per player (TEST/SERV/TEST2/TILT are on keyb)
  ConfigFile.JoyNumberOfAxes = 2;      // 2 axes per joystick
  ConfigFile.JoyNumberOfHAT = 1;       // 1 HAT per joystick
  // 8x Buttons
  for (uint8_t i = 0; i < 8; i++) {
    // P1
    ConfigFile.DigitalInB[i].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i].MapTo = (byte)i;
    // P2
    ConfigFile.DigitalInB[i + P2_DIN_OFFSET].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + P2_DIN_OFFSET].MapTo = (byte)i + (byte)(1 << 7);
  }
  // 4x HAT directions
  for (uint8_t i = 0; i < 4; i++) {
    // P1
    ConfigFile.DigitalInB[i + 8].Type = MappingType::JoyDirHAT;
    ConfigFile.DigitalInB[i + 8].MapTo = (byte)(1 << i);
    // P2
    ConfigFile.DigitalInB[i + 8 + P2_DIN_OFFSET].Type = MappingType::JoyDirHAT;
    ConfigFile.DigitalInB[i + 8 + P2_DIN_OFFSET].MapTo = (byte)(1 << i) + (byte)(1 << 7);
  }
  // 2x Coin/Start
  for (uint8_t i = 0; i < 2; i++) {
    // P1
    ConfigFile.DigitalInB[i + 12].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 12].MapTo = (byte)(i + 8);
    // P2
    ConfigFile.DigitalInB[i + 12 + P2_DIN_OFFSET].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 12 + P2_DIN_OFFSET].MapTo = (byte)(i + 8) + (byte)(1 << 7);
  }

  // Map Service buttons that are on MCU pins
  for (uint8_t i = 0; i < 4; i++) {
    ConfigFile.DigitalInB[P2_DIN_OFFSET*2+i].Type = MappingType::Key;  // TEST
  }
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+0].MapTo = KEY_F2;  // 'F2' for TEST
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+1].MapTo = KEY_F1;  // 'F1' for SERVICE
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+2].MapTo = KEY_F4;  // 'F4' for TEST2
  ConfigFile.DigitalInB[P2_DIN_OFFSET*2+3].MapTo = KEY_F3;  // 'F3' for TILT

  // 4x analog sticks on analog inputs screw terminals
  for (uint8_t i = 0; i < sizeof(ConfigFile.AnalogInDB) / sizeof(ConfigFile.AnalogInDB[0]); i++) {
    ConfigFile.AnalogInDB[i].Type = MappingType::JoyAxis;
    ConfigFile.AnalogInDB[i].MapToPos = i % 2 + ((i < 2) ? 0 : (byte)(1 << 7));  // X/Y/Z
    ConfigFile.AnalogInDB[i].MapToNeg = 0;                                       // X/Y/Z

    ConfigFile.AnalogInDB[i].DeadzoneMin = 0x60;  // dead zone min (x4) to detect axis leave center zone
    ConfigFile.AnalogInDB[i].DeadzoneMax = 0xA0;  // dead zone max (x4) to detect axis leave for center zone
  }
#endif

#if defined(USE_MOUSE) && !defined(USE_JOY) && !defined(USE_KEYB)
  // Mouse only
  Config::ConfigFile.EmulationMode = Config::EmulationModes::Mouse;
  // 5x Mouse Buttons
  for (uint8_t i = 0; i < 5; i++) {
    // P1
    ConfigFile.DigitalInB[i].Type = MappingType::MouseButton;
    ConfigFile.DigitalInB[i].MapTo = (byte)i;
    // P2
    ConfigFile.DigitalInB[i + P2_DIN_OFFSET].Type = MappingType::MouseButton;
    ConfigFile.DigitalInB[i + P2_DIN_OFFSET].MapTo = (byte)i + (byte)(1 << 7);
  }
  // Mouse axis increment from buttons
  for (uint8_t i = 0; i < 4; i++) {
    // P1 Up/Down/Left/right
    ConfigFile.DigitalInB[i + 8].Type = MappingType::MouseAxis;
    // P2 Up/Down/Left/right
    ConfigFile.DigitalInB[i + 8 + P2_DIN_OFFSET].Type = MappingType::MouseAxis;
  }
  // P1 Up/Down/Left/right
  ConfigFile.DigitalInB[8].MapTo = (byte)(0b1010);
  ConfigFile.DigitalInB[9].MapTo = (byte)(0b0010);
  ConfigFile.DigitalInB[10].MapTo = (byte)(0b1001);
  ConfigFile.DigitalInB[11].MapTo = (byte)(0b0001);
  // P2 Up/Down/Left/right
  ConfigFile.DigitalInB[8 + P2_DIN_OFFSET].MapTo = (byte)(0b1010) + (byte)(1 << 7);
  ConfigFile.DigitalInB[9 + P2_DIN_OFFSET].MapTo = (byte)(0b0010) + (byte)(1 << 7);
  ConfigFile.DigitalInB[10 + P2_DIN_OFFSET].MapTo = (byte)(0b1001) + (byte)(1 << 7);
  ConfigFile.DigitalInB[11 + P2_DIN_OFFSET].MapTo = (byte)(0b0001) + (byte)(1 << 7);
  
#endif


#ifdef DEBUG_PRINTF
  PrintConfig();
#endif
}

}