// Copyright 2015 Silicon Laboratories, Inc.                                *80*

#include "app/framework/include/af.h"
#include "relative-humidity-measurement-server.h"
#include EMBER_AF_API_HUMIDITY

#ifdef EMBER_AF_PLUGIN_REPORTING
#include "app/framework/plugin/reporting/reporting.h"
#endif

//------------------------------------------------------------------------------
// Plugin private macros

// Shorter macros for plugin options
#define MAX_HUMIDITY_MEASUREMENT_INTERVAL_MS \
(EMBER_AF_PLUGIN_RELATIVE_HUMIDITY_MEASUREMENT_SERVER_MAX_MEASUREMENT_FREQUENCY_S \
   * MILLISECOND_TICKS_PER_SECOND)

// Macro used to ensure sane humidity max/min values are stored
#define HUMIDITY_SANITY_CHECK 10000 //100.00%, in 0.01% steps = 10000

//------------------------------------------------------------------------------
// Forward Declaration of private functions
static void writeHumidityAttributes(uint16_t humidityPercentage);

//------------------------------------------------------------------------------
// Global variables
EmberEventControl emberAfPluginRelativeHumidityMeasurementServerReadEventControl;
static uint32_t humidityMeasurementIntervalMs
  = MAX_HUMIDITY_MEASUREMENT_INTERVAL_MS;

//------------------------------------------------------------------------------
// Plugin consumed callback implementations

//******************************************************************************
// Plugin init function
//******************************************************************************
void emberAfPluginRelativeHumidityMeasurementServerInitCallback(void)
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
    if (
      (entry.clusterId == ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID)
      && (entry.attributeId ==
          ZCL_RELATIVE_HUMIDITY_MEASURED_VALUE_ATTRIBUTE_ID)
      && (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED)
      && (entry.endpoint != EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID)) {
      // Max interval is set in seconds, which is the same unit of time the
      // emberAfPluginRelativeHumidityMeasurementServerSetMeasurementRate
      // expects in this API.
      emberAfPluginRelativeHumidityMeasurementServerSetMeasurementRate(
          entry.data.reported.maxInterval);
    }
  }
#endif

  emberEventControlSetActive(
    emberAfPluginRelativeHumidityMeasurementServerReadEventControl);
}

void emberAfPluginRelativeHumidityMeasurementServerStackStatusCallback(
  EmberStatus status)
{
  // On network connect, chances are very good that someone (coordinator,
  // gateway, etc) will poll the temperature for an initial status.  As such,
  // it is useful to have fresh data to be polled.
  if (status == EMBER_NETWORK_UP) {
    emberEventControlSetActive(
      emberAfPluginRelativeHumidityMeasurementServerReadEventControl);
  }
}

//------------------------------------------------------------------------------
// Plugin event handlers

//******************************************************************************
// Event used to generate a read of a new humidity value
//******************************************************************************
void emberAfPluginRelativeHumidityMeasurementServerReadEventHandler(void)
{
  halHumidityStartRead();
  emberEventControlSetInactive(
    emberAfPluginRelativeHumidityMeasurementServerReadEventControl);
}

void halHumidityReadingCompleteCallback(uint16_t humidityCentiPercent,
                                        bool readSuccess)
{
  // If the read was successful, post the results to the cluster
  if (readSuccess) {
    emberAfAppPrintln("Humidity Measurement: %2d.%2d%%",
                      (humidityCentiPercent / 100),
                      (humidityCentiPercent % 100));
    writeHumidityAttributes(humidityCentiPercent);
  } else {
    emberAfAppPrintln("Error reading humidity from HW");
  }

  emberEventControlSetDelayMS(
    emberAfPluginRelativeHumidityMeasurementServerReadEventControl,
    humidityMeasurementIntervalMs);
}

//------------------------------------------------------------------------------
// Plugin public functions

void emberAfPluginRelativeHumidityMeasurementServerSetMeasurementRate(
  uint32_t measurementIntervalS)
{
  if ((measurementIntervalS == 0)
      || (measurementIntervalS
          > EMBER_AF_PLUGIN_RELATIVE_HUMIDITY_MEASUREMENT_SERVER_MAX_MEASUREMENT_FREQUENCY_S)) {
    humidityMeasurementIntervalMs = MAX_HUMIDITY_MEASUREMENT_INTERVAL_MS;
  } else {
    humidityMeasurementIntervalMs
      = measurementIntervalS * MILLISECOND_TICKS_PER_SECOND;
  }
  emberEventControlSetDelayMS(
    emberAfPluginRelativeHumidityMeasurementServerReadEventControl,
    humidityMeasurementIntervalMs);
}

//------------------------------------------------------------------------------
// Plugin private functions

//******************************************************************************
// Update the humidity attribute of the humidity measurement cluster to
// be the humidity value given by the function's parameter.  This function
// will also query the current max and min read values, and update them if the
// given values is higher (or lower) than the previous records.
//******************************************************************************
static void writeHumidityAttributes(uint16_t humidityPercentage)
{

  uint8_t i;
  uint16_t humidityLimitPercentage;
  uint8_t endpoint;

  // Cycle through all endpoints, check to see if the endpoint has a humidity
  // server, and if so update the humidity attributes of that endpoint
  for (i = 0; i < emberAfEndpointCount(); i++) {
    endpoint = emberAfEndpointFromIndex(i);
    if (emberAfContainsServer(endpoint,
                              ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID)) {
      // Write the current humidity attribute
      emberAfWriteServerAttribute(
                    endpoint,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                    ZCL_RELATIVE_HUMIDITY_MEASURED_VALUE_ATTRIBUTE_ID,
                    (uint8_t *) &humidityPercentage,
                    ZCL_INT16U_ATTRIBUTE_TYPE);

      // Determine if this is a new minimum measured humidity, and update the
      // HUMIDITY_MIN_MEASURED attribute if that is the case.
      emberAfReadServerAttribute(
                    endpoint,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                    ZCL_RELATIVE_HUMIDITY_MIN_MEASURED_VALUE_ATTRIBUTE_ID,
                    (uint8_t *) (&humidityLimitPercentage),
                    sizeof(uint16_t) );
      if ((humidityLimitPercentage > HUMIDITY_SANITY_CHECK)
          || (humidityLimitPercentage > humidityPercentage)) {
        emberAfWriteServerAttribute(
                    endpoint,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                    ZCL_RELATIVE_HUMIDITY_MIN_MEASURED_VALUE_ATTRIBUTE_ID,
                    (uint8_t *) &humidityPercentage,
                    ZCL_INT16U_ATTRIBUTE_TYPE);
      }

      // Determine if this is a new maximum measured humidity, and update the
      // HUMIDITY_MAX_MEASURED attribute if that is the case.
      emberAfReadServerAttribute(
                    endpoint,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                    ZCL_RELATIVE_HUMIDITY_MAX_MEASURED_VALUE_ATTRIBUTE_ID,
                    (uint8_t *) (&humidityLimitPercentage),
                    sizeof(uint16_t) );
      if ((humidityLimitPercentage > HUMIDITY_SANITY_CHECK)
          || (humidityLimitPercentage < humidityPercentage)) {
        emberAfWriteServerAttribute(
                    endpoint,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                    ZCL_RELATIVE_HUMIDITY_MAX_MEASURED_VALUE_ATTRIBUTE_ID,
                    (uint8_t *) &humidityPercentage,
                    ZCL_INT16U_ATTRIBUTE_TYPE);
      }
    }
  }
}
