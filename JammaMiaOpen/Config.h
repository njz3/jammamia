/*
  Management of configuration for VideoAmp board
*/
#pragma once
// Arduino Framework
#include <Arduino.h>

// Due to code memory constraint, only enable 1x or 2x devices below
#define USE_KEYB
#define USE_JOY
//#define USE_MOUSE

//-----------------------------------------------------------------------------
// Constants and enums
//-----------------------------------------------------------------------------

//#define VERSION_NUMBER "V0.1.0.0"
#include "version.h"

#define PLATFORM_STRING "JAMMAMIAOPEN BOARD ON LEONARDO"
#define VERSION_STRING (VERSION_NUMBER " " PLATFORM_STRING)

// Protocole version reply has 16bit unsigned hex in ascii format:
// "?XXYY" where XX=major version, YY minor version
#define PROTOCOL_VERSION_MAJOR "01"
#define PROTOCOL_VERSION_MINOR "00"

// If SPRINTF is to be used instead of raw conversion
//#define USE_SPRINTF_FOR_STATUS_FRAME

// Durée d'un tick
#define TICK_MS (5UL)
#define TICK_US (TICK_MS * 1000L)
#define TICK_HZ (1000.0f / (float)TICK_MS)
#define TICK_KHZ (1.0f / (float)TICK_MS)

// Periode blink
#define BLINK_HZ (2)
// Durée blink en ms
#define BLINK_MS (1000UL / BLINK_HZ)
// Durée blink en ticks
#define BLINK_TCK (BLINK_MS / TICK_MS)

namespace Config {

// Fastest RS232 com (Leonard, Mega2560, Due)
// - 115200 is the standard hihg speed baudrate, but the
//   Mega2560@16Mhz has some timing issues (2-3% frames errors)
//   see here: http://ruemohr.org/~ircjunk/avr/baudcalc/avrbaudcalc-1.0.8.php?postbitrate=&postclock=16
// - 250000, 5000000 or 1000000 is more stable on the Mega2560
//   and other native USB like Leonardo or Due have no issues
//   whatever speed is choosen.
// => Take maximum speed 1000000 to reduce transmission delays
// Note: USB based com (Leonardo, Due) can go up to 2000000 (2Mbps)
#define PCSERIAL_BAUDRATE (1000000)

// Config options for keyboard or joystick emulation
enum EmulationModes : byte {
  NoEmulation = 0,
  // Emulation of a keyboard : a,z,w,q,e
  Keyboard = 1,
  // Emulation of 2 joysticks : X/Y axes, 8 buttons + start/coin as 2 buttons
  Joystick = 2,
  // Emulation of 2 joysticks : X/Y axes, 8 buttons + start/coin as keyboard keys
  JoystickAndKeyboard = 3,
  // Emulation of 2 mices : X/Y axes, 3 buttons per player
  Mouse = 4,
  // Emulation of 2 mices + keyboard : X/Y axes, 3 buttons + other buttons as keyboard keys
  MouseAndKeyboard = 5,
};


//-----------------------------------------------------------------------------
// Memory block definition for EPROM / RAM
//-----------------------------------------------------------------------------

// 2x14 per MCP + 4 on pro micro (test/test2/service/tilt)
#define NB_DIGITALINPUTS (32)
// 4 adc on pro-micro
#define NB_ANALOGINPUTS (4)
// 2x2 per MCP
#define NB_DIGITALOUTPUTS (4)
// 4 pwm on pro-micro
#define NB_ANALOGOUTPUTS (4)

// 4 HAT max per jostick, see HATDirections
#define MAX_HAT (4)


// Config options for keyboard or joystick emulation
enum MappingType : byte {
  Nothing = 0,
  // Emulation of a keyboard : a,z,w,q,e
  Key = 1,
  // X/Y/Z analog axes
  JoyAxis = 2,
  // 8 directions HAT (see HATDirections)
  JoyDirHAT = 3,
  // Joystick buttons
  JoyButton = 4,
  // mouse axes X/Y/Wheel from analog or digital
  MouseAxis = 5,
  // mouse button left/right/middle/prev/next
  MouseButton = 6,
};

// Config options for keyboard or joystick emulation
enum DInOptions : byte {
  None = 0,
  // Input is disabled
  Disabled = (1<<0),
  // Input is inverted
  InvertedLogic = (1<<1),
  // Repeated press
  AutoFire = (1<<2),
};

// Fixed length of an IO name
#define LENGTH_IO_NAME (3)

// Non-volatile (eeprom) digital input config, bytes field only
typedef struct __attribute__((__packed__)) {
  // Type of mapping
  MappingType Type;
  // Index of keyscan code (0 being none), joy or mouse axis, joy or mouse button
  // For axis and buttons, the 7th MSB (0b10000000) gives the player selection P1-P2, bits 6 to 0 are axis or button index
  // For HAT switch, the 7thMSB gives the player selection P1-P2, 5&6th gives the hat switch number, 3 to 0 gives the direction
  byte MapTo;
  // Index of keyscan code when using shifted/alternative map (0 being not used/none)
  byte MapToShifted;
  // Optional name
  char Name[LENGTH_IO_NAME];
} DigitalInputConfig;

// Non-volatile (eeprom) analog input config, bytes field only
typedef struct __attribute__((__packed__)) {
  // Type of mapping
  MappingType Type;
  // Index of keyscan code, joy or mouse axis, joy or mouse button
  // For axis and buttons, the 7th MSB (0b10000000) gives the player selection P1-P2, bits 6 to 0 are axis or button index
  // For HAT switch, the 7thMSB gives the player selection P1-P2, 5&6th gives the hat switch number, 3 to 0 gives the direction
  byte MapToPos;
  // Index of keyscan code, joy or mouse axis, joy or mouse button
  // For axis and buttons, the 7th MSB (0b10000000) gives the player selection P1-P2, bits 6 to 0 are axis or button index
  // For HAT switch, the 7thMSB gives the player selection P1-P2, 5&6th gives the hat switch number, 3 to 0 gives the direction
  byte MapToNeg;
  // Analog dead zone min/max (x4) for center zone of analog values, 0 (min) .. 0xFF (max)
  // Minimum value is usually 0x60 (center being 80)
  uint8_t DeadzoneMin;
  // Analog dead zone min/max (x4) for center zone of analog values, 0 (min) .. 0xFF (max)
  // Maximum value is usually 0x80 (center being 80)
  uint8_t DeadzoneMax;
  // Optional name
  char Name[LENGTH_IO_NAME];
} AnalogInputConfig;

// Non-volatile (eeprom) whole config, bytes field only
typedef struct __attribute__((__packed__)) {
  // CRC8, computed on all remaining fields below
  byte CRC8;
  // Emulation mode
  EmulationModes EmulationMode;
  // Delay for busy wait in us
  uint16_t Delay_us;
  // Emulated layout
  uint8_t KeybLayout;
  // digital inputs configuration
  DigitalInputConfig DigitalInB[NB_DIGITALINPUTS];
  // analog inputs configuration
  AnalogInputConfig AnalogInDB[NB_ANALOGINPUTS];
  // Number of joy's buttons between 0..32
  uint8_t JoyNumberOfButtons;
  // Number of joy's axes between 0..7
  uint8_t JoyNumberOfAxes;
  // Number of joy's HAT switch between 0..3 (MAX_HAT)
  uint8_t JoyNumberOfHAT;
  // index of digital input +1 that is used to use shifted/alternative map. 0 means no shifted input is configured
  uint8_t ShiftInput;
} EEPROM_CONFIG;

// ram
extern EEPROM_CONFIG ConfigFile;

//-----------------------------------------------------------------------------
// Utilities
//-----------------------------------------------------------------------------

int SaveConfigToEEPROM();
int LoadConfigFromEEPROM();
void PrintDInConfig(int);
void PrintAInConfig(int);
void PrintConfig();
void ResetConfig();

}
