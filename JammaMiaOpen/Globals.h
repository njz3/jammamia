/*
  Global variables
*/
#pragma once
#include "Config.h"

namespace Globals {

// General non-permanent config
class InternalConfig {
public:
  bool DebugMode = false;
  bool DoStreaming = false;
  bool DoEmulation = true;
};

extern InternalConfig VolatileConfig;

extern uint16_t ioReadTime_us;
extern uint16_t refreshRate_us;

// All digital inputs
extern bool DIn[NB_DIGITALINPUTS];
// All analog inputs
extern int16_t AIn[NB_ANALOGINPUTS];
// All digital outputs
extern bool DOut[NB_DIGITALOUTPUTS];
// All analog outputs
extern uint8_t AOut[NB_ANALOGOUTPUTS];
// MCU internal IOs
extern uint16_t MCUIOs;
// MCP IOs
extern uint16_t MCPIOs[2];

}