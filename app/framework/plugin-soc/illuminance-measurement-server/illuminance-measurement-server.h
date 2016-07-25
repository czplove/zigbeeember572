// Copyright 2015 Silicon Laboratories, Inc.                                *80*

#ifndef __ILLUMINANCE_MEASUREMENT_SERVER_H__
#define __ILLUMINANCE_MEASUREMENT_SERVER_H__

#define EMBER_AF_PLUGIN_ILLUMINANCE_MEASUREMENT_SERVER_MULTIPLIER_MAX   200
#define EMBER_AF_PLUGIN_ILLUMINANCE_MEASUREMENT_SERVER_MULTIPLIER_MIN   2

//------------------------------------------------------------------------------
// Plugin public function declarations

/** @brief Set the hardware read interval
 *
 * This function will set the amount of time to wait (in seconds) between polls
 * of the illuminance sensor.  This function will never set the measurement
 * interval to be greater than the plugin specified maximum measurement
 * interval.  If a value of 0 is given, the plugin specified maximum measurement
 * interval will be used for the polling interval.
 */
void emberAfPluginIlluminanceMeasurementServerSetMeasurementRate(
    uint32_t measurementRateS);

#endif //__ILLUMINANCE_MEASUREMENT_SERVER_H__
