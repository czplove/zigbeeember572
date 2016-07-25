//   Copyright 2015 by Ember Corporation. All rights reserved.              *80*

#include "app/framework/include/af.h"
#include "illuminance-measurement-server.h"
#include EMBER_AF_API_ILLUMINANCE

//******************************************************************************
// CLI function to calibrate the illuminance sensor with
// it's overlaid materials.
//******************************************************************************
void emAfIlluminanceMeasurementServerCalibrateCommand( void )
{
  uint8_t multiplier;
  uint32_t extLux = (uint32_t)emberUnsignedCommandArgument(0);
  multiplier = halIlluminanceCalibrate(extLux);
  halCommonSetToken(TOKEN_SI1141_MULTIPLIER, &multiplier);
  emberAfAppPrintln("Multiplier Set: %d", multiplier);
}
