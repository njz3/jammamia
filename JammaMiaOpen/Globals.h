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
  bool DoStreaming = true;
};

extern InternalConfig VolatileConfig;

extern uint32_t lagtimeRead;

extern uint16_t MCPIOs[2];
extern uint16_t MCUIOs;

// All digital inputs
extern bool DIn[NB_DIGITALINPUTS];
// All analog inputs
extern int16_t AIn[NB_ANALOGINPUTS];
// All digital outputs
extern bool DOut[NB_DIGITALOUTPUTS];
// All analog outputs
extern int16_t AOut[NB_ANALOGOUTPUTS];

extern bool LicenseOK;

}