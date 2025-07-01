/* JammaMia IO board
 * (c) 2023 by Bandicoot & njz3
 */

#include "config.h"
#include "Globals.h"
#include "protocol.h"
#include <Adafruit_MCP23X17.h>
#include <digitalWriteFast.h>

#ifdef USE_KEYB
#include "Keyb.h"
#endif
#ifdef USE_JOY
#include "Joy.h"
#endif
#ifdef USE_MOUSE
#include "Mou.h"
#endif

//#define DEBUG_PRINTF

//#define USE_SPI
// only used for SPI
#define CS1_PIN 10
#define CS2_PIN 16

// pin assignements for I2C on MCU
const int Interruptpin = 7;
const int SDApin = 2;
const int SCLpin = 3;

// Hardcoded pins
const int MCUDigitalInpin[4] = { 8, 16, 14, 15 };
const int MCUAnalogInpin[NB_ANALOGINPUTS] = { 18, 19, 20, 21 };
const int MCUAnalogOutpin[NB_ANALOGOUTPUTS] = { 5, 6, 9, 10 };

Adafruit_MCP23X17 mcp1;  //declaration for chip 1
Adafruit_MCP23X17 mcp2;  //declaration for chip 2

uint16_t tickCounter = 0;

void ConfigureMCUPins();

void setup() {

  ConfigureMCUPins();

  bool epromReset = false;

  // Maintain TEST2 + TILT to force reset of configuration
  epromReset = (!digitalReadFast(MCUDigitalInpin[2])) && (!digitalReadFast(MCUDigitalInpin[3]));
  // Read configuration, if that fails then reset configuration again
  epromReset |= (Config::LoadConfigFromEEPROM() != 1);

  if (epromReset) {
    Config::ResetConfig();
    Config::SaveConfigToEEPROM();
  }

  // Maintain TEST2 only to force no emulation at boot
  bool stopEmulation = (!digitalReadFast(MCUDigitalInpin[2])) && (digitalReadFast(MCUDigitalInpin[3]));
  if (stopEmulation) {
    Globals::VolatileConfig.DoEmulation = false;
  }

  Protocol::SetupPort();

#ifdef DEBUG_PRINTF
  if (epromReset) {
    Serial.println(F("MConfig internal eprom reset done"));
  } else {
    Serial.println(F("MStarting with config read from internal eprom"));
  }
#endif


#ifdef USE_SPI
  mcp1.enableAddrPins();
  if (!mcp1.begin_SPI(CS1_PIN, &SPI, 0b000) || !mcp2.begin_SPI(CS2_PIN, &SPI, 0b001)) {
#else
  // I2C
  if (!mcp1.begin_I2C(0x20, &Wire) || !mcp2.begin_I2C(0x21, &Wire)) {
#endif
    Serial.println(F("Error mcp1 or mcp2."));
  }
#ifndef USE_SPI
  // Set to maximum I2C speed for Atmega32u4
  Wire.setClock(800000L);
  /*mcp1.setupInterrupts(true, false, LOW);
  mcp1.setupInterruptPin(0, LOW);
  mcp2.setupInterrupts(true, false, LOW);
  mcp2.setupInterruptPin(0, LOW);*/
#endif

  for (int8_t i = 0; i <= 15; i++) {
    if (i == 7 || i == 15) {
      mcp1.pinMode(i, OUTPUT);
      mcp1.digitalWrite(i, false);
      mcp2.pinMode(i, OUTPUT);
      mcp2.digitalWrite(i, false);
    } else {
      mcp1.pinMode(i, INPUT_PULLUP);
      mcp2.pinMode(i, INPUT_PULLUP);
    }
  }

#ifdef DEBUG_PRINTF
  Serial.print(F("MEmulation mode="));
  Serial.println(Config::ConfigFile.EmulationMode);
#endif

  // Setup emulation
  switch (Config::ConfigFile.EmulationMode) {
#ifdef USE_KEYB
    case Config::EmulationModes::Keyboard:
      Keyb::Setup();
      break;
#endif
#ifdef USE_JOY
    case Config::EmulationModes::Joystick:
      Joy::Setup();
      break;
#endif
#ifdef USE_MOUSE
    case Config::EmulationModes::Mouse:
      Mou::Setup();
      break;
#endif
#if defined(USE_KEYB) && defined(USE_JOY)
    case Config::EmulationModes::JoystickAndKeyboard:
      Keyb::Setup();
      Joy::Setup();
      break;
#endif
#if defined(USE_KEYB) && defined(USE_MOUSE)
    case Config::EmulationModes::MouseAndKeyboard:
      Keyb::Setup();
      Mou::Setup();
      break;
#endif

    default:
    case Config::EmulationModes::NoEmulation:
      // Disabled
      break;
  }

  SetupInterrupt();
}


void MCPISR();

void SetupInterrupt() {
  // Enable HSync interrupt to start measuring HSync pulses for next 500ms
  attachInterrupt(digitalPinToInterrupt(Interruptpin), MCPISR, RISING);
}

void StopInterrupt() {
  // Disable HSync interrupt for the next 500ms
  detachInterrupt(digitalPinToInterrupt(Interruptpin));
}

// Flag to tell main loop to refresh MCP (an input triggered it)
bool doRefresh = false;
void MCPISR() {
  doRefresh = true;
  // Clear any new pending interrupt on this pin to avoid locking the arduino when
  // freq are too high
  EIFR |= (1 << INTF6);
}

void RefreshMCPInputs() {
  // Inputs are inversed (logic 0 means connected to GND = pressed)
  Globals::MCPIOs[0] = ~(mcp1.readGPIOAB());
  Globals::MCPIOs[1] = ~(mcp2.readGPIOAB());
}

void RefreshMCPOutputs(bool doutstates[4]) {
  // Inputs not inversed (logic 1 means draw current from 5V), but we are working in inverted logic due to inputs
  bitWrite(Globals::MCPIOs[0], 7, !doutstates[0]);
  bitWrite(Globals::MCPIOs[0], 15, !doutstates[1]);
  bitWrite(Globals::MCPIOs[1], 7, !doutstates[2]);
  bitWrite(Globals::MCPIOs[1], 15, !doutstates[3]);
  uint16_t gpio1 = ~(Globals::MCPIOs[0]);
  uint16_t gpio2 = ~(Globals::MCPIOs[1]);
  // Since refresh outputs over I2C is slow, alternate
  if (tickCounter % 2 == 0) {
    mcp1.writeGPIOAB(gpio1);
  } else {
    mcp2.writeGPIOAB(gpio2);
  }
}

uint16_t lastmcuio = 0;
volatile bool lastDInState[NB_DIGITALINPUTS] = {};
volatile bool DInWasShifted[NB_DIGITALINPUTS] = {};
bool isShifted = false;

void ConfigureMCUPins() {
  // Configure I2C pins on MCU
  pinMode(Interruptpin, INPUT_PULLUP);
  pinMode(SDApin, INPUT_PULLUP);
  pinMode(SCLpin, INPUT_PULLUP);

  // ADC Prescaler of 16
  ADCSRA &= ~((1 << ADPS0) + (1 << ADPS1));
  ADCSRA |= 1 << ADPS2;

  for (int i = 0; i < (int)(sizeof(MCUDigitalInpin) / sizeof(MCUDigitalInpin[0])); i++) {
    pinMode(MCUDigitalInpin[i], INPUT_PULLUP);
  }
  for (int i = 0; i < (int)(sizeof(MCUAnalogInpin) / sizeof(MCUAnalogInpin[0])); i++) {
    pinMode(MCUAnalogInpin[i], INPUT);
  }
  for (int i = 0; i < (int)(sizeof(MCUAnalogOutpin) / sizeof(MCUAnalogOutpin[0])); i++) {
    pinMode(MCUAnalogOutpin[i], INPUT_PULLUP);
  }
}

void RefreshMCUInputs() {
  // Digital
  uint16_t mcuio = 0;
  for (int i = 0; i < (int)(sizeof(MCUDigitalInpin) / sizeof(MCUDigitalInpin[0])); i++) {
    bool isPressed = !digitalReadFast(MCUDigitalInpin[i]);
    if (isPressed) {
      mcuio |= (uint16_t)1 << (uint16_t)i;
    }
  }
  Globals::MCUIOs = mcuio;
}


void ReadDIn() {
  // Update MCP inputs
  RefreshMCPInputs();
  // Update internal inputs
  RefreshMCUInputs();

  // remap from mcp din numbering to internal IO numbering 0..27
  for (int i = 0; i < 7; i++) {
    // MCP1 bits i=0..6 and j=8..14 to 0..13 (14 bits)
    volatile int j = i + 8;
    Globals::DIn[i] = bitRead(Globals::MCPIOs[0], i);      // MCP1:GPA 0..6, =1 when pressed, =0 when released
    Globals::DIn[j - 1] = bitRead(Globals::MCPIOs[0], j);  // MCP1:GPB 8..14

    Globals::DIn[i + 14] = bitRead(Globals::MCPIOs[1], i);      // MCP2:GPA 0..6
    Globals::DIn[j - 1 + 14] = bitRead(Globals::MCPIOs[1], j);  // MCP2:GPB 8..14
  }

  // remap from mcu din numbering to internal IO numbering 28..31
  for (int i = 0; i < 4; i++) {
    // MCU din 8, 16, 14, 15
    Globals::DIn[i + 28] = bitRead(Globals::MCUIOs, i);  // =1 when pressed, =0 when released
  }

  // Do we have a "shift input" configured?
  if (Config::ConfigFile.ShiftInput > 0) {
    // Check the input state
    if (Globals::DIn[Config::ConfigFile.ShiftInput - 1]) {
      // Save "shifted" state
      isShifted = true;
    } else {
      isShifted = false;
    }
  }
}

void ReadAIn() {
  for (int i = 0; i < NB_ANALOGINPUTS; i++) {
    Globals::AIn[i] = analogRead(MCUAnalogInpin[i]);
  }
}

void WriteDOut() {
  // Simply transfer to mcp for the 4 outputs
  RefreshMCPOutputs(Globals::DOut);
}

void WriteAOut() {
  // Write mcu pwm output
  for (int i = 0; i < NB_ANALOGOUTPUTS; i++) {
    analogWrite(MCUAnalogOutpin[i], Globals::AOut[i]);
  }
}

void ProcessDigitalInput(int index, bool newstate) {
  auto dinDB = Config::ConfigFile.DigitalInB[index];
  byte map = dinDB.MapTo;
  if (newstate) {
    // pressed
    if (isShifted && (dinDB.MapToShifted > 0)) {
      DInWasShifted[index] = true;
      map = dinDB.MapToShifted;
    } else {
      DInWasShifted[index] = false;
    }
  } else {
    // released
    if (DInWasShifted[index]) {
      map = dinDB.MapToShifted;
    }
  }

#ifdef DEBUG_PRINTF
  Serial.print(F("din "));
  Serial.print(index, HEX);
  Serial.print(F(" type "));
  Serial.print(dinDB.Type, HEX);
  Serial.print(F(" state "));
  Serial.print(newstate, HEX);
  Serial.print(F(" mapto 0x"));
  Serial.println(map, HEX);
#endif

  switch (dinDB.Type) {
#ifdef USE_KEYB
    case Config::MappingType::Key:
      {
        if (newstate) {
          Keyb::Press(map);
        } else {
          Keyb::Release(map);
        }
      }
      break;
#endif
#ifdef USE_JOY
    case Config::MappingType::JoyButton:
      {
        if (newstate) {
          Joy::BtnPress(map);
        } else {
          Joy::BtnRelease(map);
        }
      }
      break;
    case Config::MappingType::JoyDirHAT:
      {
        if (newstate) {
          Joy::SetHATSwitch(map, true);
        } else {
          Joy::SetHATSwitch(map, false);
        }
      }
      break;
#endif
#ifdef USE_MOUSE
    case Config::MappingType::MouseButton:
      {
        if (newstate) {
          Mou::BtnPress(map);
        } else {
          Mou::BtnRelease(map);
        }
      }
      break;
    case Config::MappingType::MouseAxisIncr:
      {
        if (newstate) {
          Mou::BtnPress(map);
        } else {
          Mou::BtnRelease(map);
        }
      }
      break;
#endif

    default:
      break;
  }
}


// index in 0..3
// value is between 0 and 1023 (0x3FF). middle point being 511 (0x1FF)
// Threasholds for center and middle deadzone : 0x100 and 0x300
void ProcessAnalogInput(int index, int value) {
  auto ainDB = Config::ConfigFile.AnalogInDB[index];
  int16_t min = ((int16_t)ainDB.DeadzoneMin) << 2;
  int16_t max = ((int16_t)ainDB.DeadzoneMax) << 2;

  switch (ainDB.Type) {
#ifdef USE_KEYB
    case Config::MappingType::Key:
      {
        // Do thing when input is configured for keyboard
        if (value < min) {
          Keyb::Press(ainDB.MapToNeg);
        } else {
          Keyb::Release(ainDB.MapToNeg);
        }
        if (value > max) {
          Keyb::Press(ainDB.MapToPos);
        } else {
          Keyb::Release(ainDB.MapToPos);
        }
      }
      break;
#endif
#ifdef USE_JOY
    case Config::MappingType::JoyAxis:
      {
        // Only positive mapping is used for analog axes
        Joy::SetAxis(ainDB.MapToPos, value);
      }
      break;
    case Config::MappingType::JoyButton:
      {
        if (value < min) {
          Joy::BtnPress(ainDB.MapToNeg);
          Joy::BtnRelease(ainDB.MapToPos);
        } else if (value > max) {
          Joy::BtnRelease(ainDB.MapToNeg);
          Joy::BtnPress(ainDB.MapToPos);
        } else {
          Joy::BtnRelease(ainDB.MapToNeg);
          Joy::BtnRelease(ainDB.MapToPos);
        }
      }
      break;
    case Config::MappingType::JoyDirHAT:
      {
        if (value < min) {
          Joy::SetHATSwitch(ainDB.MapToNeg, true);
          Joy::SetHATSwitch(ainDB.MapToPos, false);
        } else if (value > max) {
          Joy::SetHATSwitch(ainDB.MapToNeg, false);
          Joy::SetHATSwitch(ainDB.MapToPos, true);
        } else {
          Joy::SetHATSwitch(ainDB.MapToNeg, false);
          Joy::SetHATSwitch(ainDB.MapToPos, false);
        }
      }
      break;
#endif
#ifdef USE_MOUSE
    case Config::MappingType::MouseAxis:
      {
        Mou::SetAxis(ainDB.MapToPos, value);
      }
      break;
    case Config::MappingType::MouseButton:
      {
        if (value < min) {
          Mou::BtnPress(ainDB.MapToNeg);
          Mou::BtnRelease(ainDB.MapToPos);
        } else if (value > max) {
          Mou::BtnRelease(ainDB.MapToNeg);
          Mou::BtnPress(ainDB.MapToPos);
        } else {
          Mou::BtnRelease(ainDB.MapToNeg);
          Mou::BtnRelease(ainDB.MapToPos);
        }
      }
      break;
#endif

    default:
      break;
  }
}



void RefreshIOs() {
  uint32_t start = micros();
  // Refresh to Globals::
  ReadDIn();
  ReadAIn();
  WriteDOut();
  WriteAOut();
  uint32_t end = micros();
  Globals::ioReadTime_us = end - start;

  // Loop on Globals
  for (int i = 0; i < NB_DIGITALINPUTS; i++) {
    // Detect change
    if (lastDInState[i] ^ Globals::DIn[i]) {
      ProcessDigitalInput(i, Globals::DIn[i]);
      lastDInState[i] = Globals::DIn[i];
    }
#ifdef DEBUG_PRINTF
    Serial.print(" ");
    Serial.print(Globals::DIn[i], HEX);
#endif
  }
  for (int i = 0; i < NB_ANALOGINPUTS; i++) {
    ProcessAnalogInput(i, Globals::AIn[i]);
  }
}

void doEmulation() {
  switch (Config::ConfigFile.EmulationMode) {
    case Config::EmulationModes::NoEmulation:
      // Disabled
      break;
    case Config::EmulationModes::Keyboard:
#ifdef USE_KEYB
      Keyb::UpdateToPC();
#endif
      break;
    case Config::EmulationModes::Joystick:
#ifdef USE_JOY
      Joy::UpdateToPC();
#endif
      break;
    case Config::EmulationModes::Mouse:
#ifdef USE_MOUSE
      Mou::UpdateToPC();
#endif
      break;
    case Config::EmulationModes::JoystickAndKeyboard:
#if defined(USE_KEYB) && defined(USE_JOY)
      Keyb::UpdateToPC();
      Joy::UpdateToPC();
#endif
      break;
    case Config::EmulationModes::MouseAndKeyboard:
#if defined(USE_KEYB) && defined(USE_MOUSE)
      Keyb::UpdateToPC();
      Mou::UpdateToPC();
#endif
      break;
  }
}



uint32_t lastrun_us = 0;
void loop() {

  //---------------------------------------------------------------------------
  // Throttle
  //---------------------------------------------------------------------------

  // Make the loop rate close to selected delay
  uint32_t now_us = micros();
  Globals::refreshRate_us = (uint16_t)(now_us - lastrun_us);
  lastrun_us = now_us;

  //---------------------------------------------------------------------------
  // IOs
  //---------------------------------------------------------------------------

  // Read/Write all IOs
  RefreshIOs();

  //---------------------------------------------------------------------------
  // Emulation of keyboard/mouse/joystick
  //---------------------------------------------------------------------------

  // Perform emulation at a slower loop rate
  //if ((tickCounter % 2) == 0) {
  if (Globals::VolatileConfig.DoEmulation) {
    doEmulation();
  }
  //}


  //---------------------------------------------------------------------------
  // Communication
  //---------------------------------------------------------------------------

  // Send status frames every 100ms
  tickCounter++;
  if ((tickCounter % 500) == 0) {
    if (Globals::VolatileConfig.DoStreaming) {
      Protocol::SendStatusFrame();
    }
  }

  // Process serial command
  Protocol::ProcessOneMessage();

  // Arduino stuff will run after this method
  if (Config::ConfigFile.Delay_us > 0) {
    delayMicroseconds(Config::ConfigFile.Delay_us);
  }
}
