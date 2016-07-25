// Copyright 2015 Silicon Laboratories, Inc.                                *80*

#include "app/framework/include/af.h"
#include "illuminance-measurement-server.h"
#include EMBER_AF_API_ILLUMINANCE

#ifdef EMBER_AF_PLUGIN_REPORTING 
#include "app/framework/plugin/reporting/reporting.h"
#endif

//------------------------------------------------------------------------------
// Plugin private macros

// Shorter macros for plugin options
#define MAX_MEASUREMENT_FREQUENCY_MS \
  (EMBER_AF_PLUGIN_ILLUMINANCE_MEASUREMENT_SERVER_MAX_MEASUREMENT_FREQUENCY_S \
   * MILLISECOND_TICKS_PER_SECOND)

//------------------------------------------------------------------------------
// Forward Declaration of private functions
static void writeIlluminanceAttributes(uint16_t illuminanceLogLx);

//------------------------------------------------------------------------------
// Global variables
EmberEventControl emberAfPluginIlluminanceMeasurementServerReadEventControl;
static uint32_t illuminanceMeasurementRateMS = MAX_MEASUREMENT_FREQUENCY_MS;

//------------------------------------------------------------------------------
// Plugin consumed callback implementations

//******************************************************************************
// Plugin init function
//******************************************************************************
void emberAfPluginIlluminanceMeasurementServerInitCallback(void)
{
#ifdef EMBER_AF_PLUGIN_REPORTING 
  uint8_t i;

  EmberAfPluginReportingEntry entry;

  // On initialization, cycle through the reporting table to determine if a
  // reporting interval was set for the device before powering down.  If so,
  // update the sensor's hardware polling rate to match the attribute defined
  // maxInterval.  Otherwise, the plugin will use the plugin's option defined
  // default hardware polling interval.
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    emAfPluginReportingGetEntry(i, &entry);
    if ((entry.clusterId == ZCL_ILLUM_MEASUREMENT_CLUSTER_ID)
        && (entry.attributeId == ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID)
        && (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED)
        && (entry.endpoint != EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID)) {
      // Max interval is set in seconds, which is the same unit of time the
      // emberAfPluginIlluminanceMeasurementServerSetMeasurementRate expects in
      // this API.
      emberAfPluginIlluminanceMeasurementServerSetMeasurementRate(
        entry.data.reported.maxInterval);
    }
  }
#endif

  // Start the ReadEvent, which will re-activate itself perpetually
  emberEventControlSetActive(
      emberAfPluginIlluminanceMeasurementServerReadEventControl);
}

void emberAfPluginIlluminanceMeasurementServerStackStatusCallback(
  EmberStatus status)
{
  // On network connect, chances are very good that someone (coordinator,
  // gateway, etc) will poll the illuminance for an initial status.  As such,
  // it is useful to have fresh data to be polled.
  if (status == EMBER_NETWORK_UP) {
    emberEventControlSetActive(
      emberAfPluginIlluminanceMeasurementServerReadEventControl);
  }
}

//------------------------------------------------------------------------------
// Plugin event handlers

//******************************************************************************
// Event used to generate a read of a new illuminance value
//******************************************************************************
void emberAfPluginIlluminanceMeasurementServerReadEventHandler(void)
{
  uint8_t multiplier;

  halCommonGetToken(&multiplier, TOKEN_SI1141_MULTIPLIER);

  // sanity check for mulitplier
  if ((multiplier
       < EMBER_AF_PLUGIN_ILLUMINANCE_MEASUREMENT_SERVER_MULTIPLIER_MIN)
      || (multiplier
          > EMBER_AF_PLUGIN_ILLUMINANCE_MEASUREMENT_SERVER_MULTIPLIER_MAX)) {
    multiplier = 0; // use default value instead
  }
  halIlluminanceStartRead(multiplier);
  emberEventControlSetInactive(
    emberAfPluginIlluminanceMeasurementServerReadEventControl);
}

void halIlluminanceReadingCompleteCallback(uint16_t logLux)
{
  emberAfAppPrintln("Illuminance: %d", logLux);
  writeIlluminanceAttributes(logLux);
  emberEventControlSetDelayMS(
    emberAfPluginIlluminanceMeasurementServerReadEventControl,
    illuminanceMeasurementRateMS);
}
//------------------------------------------------------------------------------
// Plugin public functions

void emberAfPluginIlluminanceMeasurementServerSetMeasurementRate(
  uint32_t measurementRateS)
{
  if ((measurementRateS == 0)
      || (measurementRateS
          > EMBER_AF_PLUGIN_ILLUMINANCE_MEASUREMENT_SERVER_MAX_MEASUREMENT_FREQUENCY_S)) {
    illuminanceMeasurementRateMS = MAX_MEASUREMENT_FREQUENCY_MS;
  } else {
    illuminanceMeasurementRateMS
      = measurementRateS * MILLISECOND_TICKS_PER_SECOND;
  }
  emberEventControlSetDelayMS(
    emberAfPluginIlluminanceMeasurementServerReadEventControl,
    illuminanceMeasurementRateMS);
}

//------------------------------------------------------------------------------
// Plugin private functions

//******************************************************************************
// Update the illuminance attribute of the illuminance measurement cluster to
// be the illuminance value given by the function's parameasurement. This
// function will also query the current max and min read values, and update
// them if the given values is higher (or lower) than the previous records.
//******************************************************************************
static void writeIlluminanceAttributes(uint16_t illuminanceLogLx)
{
  uint16_t illumLimitLogLx;

  uint8_t i;
  uint8_t endpoint;

  // Cycle through all endpoints, check to see if the endpoint has a illuminance
  // server, and if so update the illuminance attributes of that endpoint
  for (i = 0; i < emberAfEndpointCount(); i++) {
    endpoint = emberAfEndpointFromIndex(i);
    if (emberAfContainsServer(endpoint, ZCL_ILLUM_MEASUREMENT_CLUSTER_ID )) {
      emberAfIllumMeasurementClusterPrintln(
                         "Illuminance Measurement(LogLux):%d",
                         illuminanceLogLx);
      //Write the current illuminance attribute
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_ILLUM_MEASUREMENT_CLUSTER_ID ,
                                  ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID,
                                  (uint8_t *) &illuminanceLogLx,
                                  ZCL_INT16U_ATTRIBUTE_TYPE);

      // Determine if this is a new minimum measured illuminance, and update the
      // ILLUM_MIN_MEASURED attribute if that is the case.
      emberAfReadServerAttribute(endpoint,
                                 ZCL_ILLUM_MEASUREMENT_CLUSTER_ID ,
                                 ZCL_ILLUM_MIN_MEASURED_VALUE_ATTRIBUTE_ID,
                                 (uint8_t *) (&illumLimitLogLx),
                                 sizeof(uint16_t) );
      if ((illumLimitLogLx > illuminanceLogLx)) {
        emberAfWriteServerAttribute(endpoint,
                                    ZCL_ILLUM_MEASUREMENT_CLUSTER_ID ,
                                    ZCL_ILLUM_MIN_MEASURED_VALUE_ATTRIBUTE_ID,
                                    (uint8_t *) &illuminanceLogLx,
                                    ZCL_INT16U_ATTRIBUTE_TYPE);
      }

      // Determine if this is a new maximum measured illuminance, and update the
      // ILLUM_MAX_MEASURED attribute if that is the case.
      emberAfReadServerAttribute(endpoint,
                                 ZCL_ILLUM_MEASUREMENT_CLUSTER_ID ,
                                 ZCL_ILLUM_MAX_MEASURED_VALUE_ATTRIBUTE_ID,
                                 (uint8_t *)(&illumLimitLogLx),
                                 sizeof(uint16_t) );
      if ((illumLimitLogLx < illuminanceLogLx)) {
        emberAfWriteServerAttribute(endpoint,
                                    ZCL_ILLUM_MEASUREMENT_CLUSTER_ID ,
                                    ZCL_ILLUM_MAX_MEASURED_VALUE_ATTRIBUTE_ID,
                                    (uint8_t *) &illuminanceLogLx,
                                    ZCL_INT16U_ATTRIBUTE_TYPE);
      }
    }
  }
}
