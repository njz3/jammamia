/*
  Management of configuration for IO board
*/
#include "Config.h"
#include <EEPROM.h>
#include "CRC.h"
#include "Globals.h"

#ifdef USE_KEYB

#ifdef ARDUINO_AVR_LEONARDO
#include "KeyboardNKey.h"
#else
#include <Keyboard.h>
#define KeyboardNKey Keyboard
#endif

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

// Reset to default values
void ResetConfig() {
  ConfigFile.SerialSpeed = PCSERIAL_BAUDRATE;

#ifdef USE_JOY
  Config::ConfigFile.EmulationMode = Config::EmulationModes::Joystick;
#else
#ifdef USE_KEY
  Config::ConfigFile.EmulationMode = Config::EmulationModes::Keyboard;
#endif
#endif

  ConfigFile.Delay_us = 0;

  // Emulated layout
  //ConfigFile.KeybLayout = 0;  // Layout en-US
  ConfigFile.KeybLayout = 1;  // Layout fr-FR

// Map Player1 on MCP1
#ifdef USE_KEYB
  // Emulated keys for all digital inputs
  for (uint8_t i = 0; i < sizeof(ConfigFile.DigitalInB) / sizeof(ConfigFile.DigitalInB[0]); i++) {
    ConfigFile.DigitalInB[i].Type = MappingType::Key;
  }

  // Emulated keys, using MAME default layout
  ConfigFile.DigitalInB[0].MapTo = KEY_LEFT_CTRL;     // P1-But1
  ConfigFile.DigitalInB[1].MapTo = KEY_LEFT_ALT;      // P1-But2
  ConfigFile.DigitalInB[2].MapTo = ' ';               // P1-But3
  ConfigFile.DigitalInB[3].MapTo = KEY_LEFT_SHIFT;    // P1-But4
  ConfigFile.DigitalInB[4].MapTo = 'z';               // P1-But5
  ConfigFile.DigitalInB[5].MapTo = 'x';               // P1-But6
  ConfigFile.DigitalInB[6].MapTo = 'c';               // P1-But7
  ConfigFile.DigitalInB[7].MapTo = 'v';               // P1-But8
  ConfigFile.DigitalInB[8].MapTo = KEY_UP_ARROW;      // P1-Up
  ConfigFile.DigitalInB[9].MapTo = KEY_DOWN_ARROW;    // P1-Down
  ConfigFile.DigitalInB[10].MapTo = KEY_LEFT_ARROW;   // P1-Left
  ConfigFile.DigitalInB[11].MapTo = KEY_RIGHT_ARROW;  // P1-Right
  ConfigFile.DigitalInB[12].MapTo = '5';              // '5' for COIN1
  ConfigFile.DigitalInB[13].MapTo = '1';              // '1' for START1

  // Map Player2 on MCP2
  ConfigFile.DigitalInB[14].MapTo = 'a';  // P2-But1
  ConfigFile.DigitalInB[15].MapTo = 's';  // P2-But2
  ConfigFile.DigitalInB[16].MapTo = 'q';  // P2-But3
  ConfigFile.DigitalInB[17].MapTo = 'w';  // P2-But4
  ConfigFile.DigitalInB[18].MapTo = 'e';  // P2-But5
  ConfigFile.DigitalInB[19].MapTo = 'b';  // P2-But6
  ConfigFile.DigitalInB[20].MapTo = 'n';  // P2-But7
  ConfigFile.DigitalInB[21].MapTo = 'm';  // P2-But8

  ConfigFile.DigitalInB[22].MapTo = 'r';  // P2-Up
  ConfigFile.DigitalInB[23].MapTo = 'f';  // P2-Down
  ConfigFile.DigitalInB[24].MapTo = 'd';  // P2-Left
  ConfigFile.DigitalInB[25].MapTo = 'g';  // P2-Right
  ConfigFile.DigitalInB[26].MapTo = '6';  // '6' for COIN2
  ConfigFile.DigitalInB[27].MapTo = '2';  // '1' for START2

  // Map Service buttons that are on MCU pins
  ConfigFile.DigitalInB[28].MapTo = KEY_F2;  // 'F2' for TEST
  ConfigFile.DigitalInB[29].MapTo = '9';     // '9' for SERVICE
  ConfigFile.DigitalInB[30].MapTo = KEY_F4;  // 'F4' for TEST2
  ConfigFile.DigitalInB[31].MapTo = 't';     // 't' for TILT
#endif

#ifdef USE_MOUSE
  // 3x Mouse Buttons
  for (uint8_t i = 0; i < 3; i++) {
    // P1
    ConfigFile.DigitalInB[i].Type = MappingType::MouseButton;
    ConfigFile.DigitalInB[i].MapTo = (byte)i;
    // P2
    ConfigFile.DigitalInB[i + 14].Type = MappingType::MouseButton;
    ConfigFile.DigitalInB[i + 14].MapTo = (byte)i + (byte)(1 << 7);
  }
  // Mouse axis increment
  for (uint8_t i = 0; i < 4; i++) {
    // P1 Up/Down/Left/right
    ConfigFile.DigitalInB[i + 8].Type = MappingType::MouseAxisIncr;
    ConfigFile.DigitalInB[i + 8].MapTo = (byte)(1 << i);
    // P2
    ConfigFile.DigitalInB[i + 8 + 14].Type = MappingType::MouseAxisIncr;
    ConfigFile.DigitalInB[i + 8 + 14].MapTo = (byte)(1 << i) + (byte)(1 << 7);
  }
  
#endif


#ifdef USE_JOY
  ConfigFile.JoyNumberOfButtons = 10;  // 8+2
  ConfigFile.JoyNumberOfAxes = 2;      // 2 axes per joystick
  ConfigFile.JoyNumberOfHAT = 1;       // 1 HAT per joystick
  // 8x Buttons
  for (uint8_t i = 0; i < 8; i++) {
    // P1
    ConfigFile.DigitalInB[i].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i].MapTo = (byte)i;
    // P2
    ConfigFile.DigitalInB[i + 14].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 14].MapTo = (byte)i + (byte)(1 << 7);
  }
  // 4x HAT directions
  for (uint8_t i = 0; i < 4; i++) {
    // P1
    ConfigFile.DigitalInB[i + 8].Type = MappingType::JoyDirHAT;
    ConfigFile.DigitalInB[i + 8].MapTo = (byte)(1 << i);
    // P2
    ConfigFile.DigitalInB[i + 8 + 14].Type = MappingType::JoyDirHAT;
    ConfigFile.DigitalInB[i + 8 + 14].MapTo = (byte)(1 << i) + (byte)(1 << 7);
  }
  // 2x Start/coin
  for (uint8_t i = 0; i < 2; i++) {
    // P1
    ConfigFile.DigitalInB[i + 12].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 12].MapTo = (byte)(i + 8);
    // P2
    ConfigFile.DigitalInB[i + 12 + 14].Type = MappingType::JoyButton;
    ConfigFile.DigitalInB[i + 12 + 14].MapTo = (byte)(i + 8) + (byte)(1 << 7);
  }
  // 4x analog sticks
  for (uint8_t i = 0; i < sizeof(ConfigFile.AnalogInDB) / sizeof(ConfigFile.AnalogInDB[0]); i++) {
    ConfigFile.AnalogInDB[i].Type = MappingType::JoyAxis;
    ConfigFile.AnalogInDB[i].MapToPos = i%2 + ((i<2)?0:(byte)(1 << 7));  // X/Y/Z
    ConfigFile.AnalogInDB[i].MapToNeg = 0;  // X/Y/Z

    ConfigFile.AnalogDeadzoneMinMax[i][0] = 0x60;
    ConfigFile.AnalogDeadzoneMinMax[i][1] = 0x80;
  }

#endif
  /*
  for (uint8_t i = 0; i < sizeof(ConfigFile.DigitalInB) / sizeof(ConfigFile.DigitalInB[0]); i++) {
    Serial.print(i);
    Serial.print(" type="); Serial.print(ConfigFile.DigitalInB[i].Type);
    Serial.print(" mapto="); Serial.print(ConfigFile.DigitalInB[i].MapTo, HEX);
    Serial.println();
  }
  */
}

}