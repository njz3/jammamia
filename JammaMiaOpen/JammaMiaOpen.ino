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

void setup() {

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
    pinMode(MCUAnalogInpin[i], INPUT_PULLUP);
  }
  for (int i = 0; i < (int)(sizeof(MCUAnalogOutpin) / sizeof(MCUAnalogOutpin[0])); i++) {
    pinMode(MCUAnalogOutpin[i], INPUT_PULLUP);
  }

  bool epromResetDone = false;
  // Read configuration, if that fails then reset configuration again
  if (Config::LoadConfigFromEEPROM() != 1) {
    Config::ResetConfig();
    Config::SaveConfigToEEPROM();
    epromResetDone = true;
  }

  Protocol::SetupPort();

  if (epromResetDone) {
    Serial.println(F("MConfig internal eprom reset done"));
  } else {
    Serial.println(F("MStarting with config read from internal eprom"));
  }


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
      mcp1.digitalWrite(i, true);
      mcp2.pinMode(i, OUTPUT);
      mcp2.digitalWrite(i, true);
    } else {
      mcp1.pinMode(i, INPUT_PULLUP);
      mcp2.pinMode(i, INPUT_PULLUP);
    }
  }

  Config::ConfigFile.EmulationMode = Config::EmulationModes::MouseAndKeyboard;
  Serial.print(F("MEmulation mode="));
  Serial.println(Config::ConfigFile.EmulationMode);

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
      Joy::Setup();
      Keyb::Setup();
      break;
#endif
#if defined(USE_KEYB) && defined(USE_MOUSE)
    case Config::EmulationModes::MouseAndKeyboard:
      Mou::Setup();
      Keyb::Setup();
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
  Globals::MCPIOs[0] = ~(mcp1.readGPIOAB());
  Globals::MCPIOs[1] = ~(mcp2.readGPIOAB());
}

void RefreshMCPOutputs(bool doutstates[4]) {
  bitWrite(Globals::MCPIOs[0], 7, doutstates[0]);
  bitWrite(Globals::MCPIOs[0], 15, doutstates[1]);
  bitWrite(Globals::MCPIOs[1], 7, doutstates[2]);
  bitWrite(Globals::MCPIOs[1], 15, doutstates[3]);
  mcp1.writeGPIOAB(Globals::MCPIOs[0]);
  mcp2.writeGPIOAB(Globals::MCPIOs[1]);
}

uint16_t lastmcuio = 0;

void RefreshMCUInputs() {
  // Digital
  uint16_t mcuio = 0;
  for (int i = 0; i < 4; i++) {
    mcuio |= (digitalReadFast(MCUDigitalInpin[i]) == 0) << i;
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
    Globals::DIn[i] = bitRead(Globals::MCPIOs[0], i);       // MCP1:GPA 0..6, =1 when pressed, =0 when released
    Globals::DIn[j-1] = bitRead(Globals::MCPIOs[0], j);       // MCP1:GPB 8..14

    Globals::DIn[i + 14] = bitRead(Globals::MCPIOs[1], i);  // MCP2:GPA 0..6
    Globals::DIn[j-1 + 14] = bitRead(Globals::MCPIOs[1], j);  // MCP2:GPB 8..14
  }

  // remap from mcu din numbering to internal IO numbering 28..31
  for (int i = 0; i < 4; i++) {
    // MCU din 8, 16, 14, 15
    Globals::DIn[i + 28] = bitRead(Globals::MCUIOs, i);  // =1 when pressed, =0 when released
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
  Serial.print("din ");
  Serial.print(index, HEX);
  Serial.print(" type ");
  Serial.print(dinDB.Type, HEX);
  Serial.print(" state ");
  Serial.print(newstate, HEX);
  Serial.print(" mapto ");
  Serial.println(dinDB.MapTo, HEX);
  
  switch (dinDB.Type) {
    case Config::MappingType::Key:
      {
#ifdef USE_KEYB
        if (newstate) {
          Keyb::Press(dinDB.MapTo);
        } else {
          Keyb::Release(dinDB.MapTo);
        }
#endif
      }
      break;
    case Config::MappingType::JoyButton:
      {
#ifdef USE_JOY
        if (newstate) {
          Joy::BtnPress(dinDB.MapTo);
        } else {
          Joy::BtnRelease(dinDB.MapTo);
        }
#endif
      }
      break;
    case Config::MappingType::JoyDirHAT:
      {
#ifdef USE_JOY
        if (newstate) {
          Joy::SetHATSwitch(dinDB.MapTo, true);
        } else {
          Joy::SetHATSwitch(dinDB.MapTo, false);
        }
#endif
      }
      break;
    case Config::MappingType::MouseButton:
      {
#ifdef USE_MOUSE
        if (newstate) {
          Mou::BtnPress(dinDB.MapTo);
        } else {
          Mou::BtnRelease(dinDB.MapTo);
        }
#endif
      }
      break;
      case Config::MappingType::MouseAxisIncr:
      {
#ifdef USE_MOUSE
        if (newstate) {
          Mou::BtnPress(dinDB.MapTo);
        } else {
          Mou::BtnRelease(dinDB.MapTo);
        }
#endif
      }
      break;

    default:
      break;
  }
}


// index in 0..3
// value is between 0 and 1023 (0x3FF). middle point being 511 (0x1FF)
// Threasholds for center and middle deadzone : 0x100 and 0x300
void ProcessAnalogInput(int index, int value) {
  auto ainDB = Config::ConfigFile.AnalogInDB[index];
  switch (ainDB.Type) {
    case Config::MappingType::Key:
      {
        // Do thing when input is configured for keyboard
      }
      break;
    case Config::MappingType::JoyAxis:
      {
#ifdef USE_JOY
        Joy::SetAxis(ainDB.MapToPos, value);
#endif
      }
      break;
    case Config::MappingType::JoyButton:
      {
#ifdef USE_JOY
        int16_t min = ((int16_t)Config::ConfigFile.AnalogDeadzoneMinMax[index][0]) << 2;
        int16_t max = ((int16_t)Config::ConfigFile.AnalogDeadzoneMinMax[index][1]) << 2;
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
#endif
      }
      break;
    case Config::MappingType::JoyDirHAT:
      {
#ifdef USE_JOY
        int16_t min = ((int16_t)Config::ConfigFile.AnalogDeadzoneMinMax[index][0]) << 2;
        int16_t max = ((int16_t)Config::ConfigFile.AnalogDeadzoneMinMax[index][1]) << 2;
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
#endif
      }
      break;
    case Config::MappingType::MouseAxis:
      {
#ifdef USE_MOUSE
        Mou::SetAxis(ainDB.MapToPos, value);
#endif
      }
      break;
    case Config::MappingType::MouseButton:
      {
#ifdef USE_MOUSE
        int16_t min = ((int16_t)Config::ConfigFile.AnalogDeadzoneMinMax[index][0]) << 2;
        int16_t max = ((int16_t)Config::ConfigFile.AnalogDeadzoneMinMax[index][1]) << 2;
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
#endif
      }
      break;

    default:
      break;
  }
}



volatile bool lastDInState[NB_DIGITALINPUTS] = {};

void RefreshIOs() {
  uint32_t start = micros();
  // Refresh to Globals::
  ReadDIn();
  WriteDOut();
  ReadAIn();
  WriteAOut();
  uint32_t end = micros();
  Globals::lagtimeRead = end - start;

  // Loop on Globals
  for (int i = 0; i < NB_DIGITALINPUTS; i++) {
    if (lastDInState[i] ^ Globals::DIn[i]) {
      ProcessDigitalInput(i, Globals::DIn[i]);
      lastDInState[i] = Globals::DIn[i];
    }
    /*Serial.print(" ");
    Serial.print(Globals::DIn[i], HEX);*/
  }
  //Serial.println("");
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



uint16_t tickCounter = 0;
void loop() {
  // Current time
  //uint32_t now_ms = millis();

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

  // Read/Write all IOs
  RefreshIOs();

  //---------------------------------------------------------------------------
  // Emulation of keyboard/mouse/joystick
  //---------------------------------------------------------------------------

  // Perform emulation at a slower loop rate
  //if ((tickCounter % 2) == 0) {
    doEmulation();
  //}


  // Throttle the loop to make it close to 1 ms
  delayMicroseconds(100);
}
