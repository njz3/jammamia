/*
  Global variables
*/
#include "Globals.h"

namespace Globals {

InternalConfig VolatileConfig;

uint32_t lagtimeRead = 0;

// Bit order GPA 0..6, GPB 0..6
uint16_t MCPIOs[2];
// Bit order: 8, 16, 14, 15
uint16_t MCUIOs; 


// All digital inputs
bool DIn[NB_DIGITALINPUTS];
// All analog inputs
int16_t AIn[NB_ANALOGINPUTS];
// All digital outputs
bool DOut[NB_DIGITALOUTPUTS];
// All analog outputs
int16_t AOut[NB_ANALOGOUTPUTS];

bool LicenseOK = false;

}
