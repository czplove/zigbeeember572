// Copyright 2015 Silicon Laboratories, Inc.                                *80*

#include "app/framework/include/af.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "app/framework/plugin-soc/illuminance-measurement-server/illuminance-measurement-server.h"
#include "app/framework/plugin-soc/relative-humidity-measurement-server/relative-humidity-measurement-server.h"
#include "app/framework/plugin-soc/temperature-measurement-server/temperature-measurement-server.h"
#include "app/framework/plugin-soc/connection-manager/connection-manager.h"

//------------------------------------------------------------------------------
// Application specific global variables

// This callback will execute any time the reporting intervals are modified.
// In order to verify the occupancy sensor is polling the environment sensors
// frequently enough for the report intervals to be effective, it is necessary
// to call the SetMeasurementRate function for each sensor any time the
// reporting intervals are changed.
EmberAfStatus emberAfPluginReportingConfiguredCallback(
  const EmberAfPluginReportingEntry *entry)
{
  if (entry->direction != EMBER_ZCL_REPORTING_DIRECTION_REPORTED) {
    return EMBER_ZCL_STATUS_SUCCESS;
  }

  if ((entry->clusterId == ZCL_TEMP_MEASUREMENT_CLUSTER_ID)
      && (entry->attributeId == ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID)) {

    // If setMeasurementRate is called with a value of 0, the hardware will
    // revert to polling the hardware at the maximum rate, specified by the HAL
    // plugin.
    if (entry->endpoint == EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID) {
      emberAfCorePrintln("Temperature reporting disabled");
      emberAfPluginTemperatureMeasurementServerSetMeasurementRate(0);
    } else {

      //Max interval is set in seconds, which is the same unit of time the
      //emberAfPluginTemperatureMeasurementServerSetMeasurementRate expects in
      //this API.
      emberAfCorePrintln("Temperature reporting interval set: %d seconds",
                          entry->data.reported.maxInterval);
      emberAfPluginTemperatureMeasurementServerSetMeasurementRate(
        entry->data.reported.maxInterval);
    }
  } else if ((entry->clusterId == ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID)
             && (entry->attributeId
                 == ZCL_RELATIVE_HUMIDITY_MEASURED_VALUE_ATTRIBUTE_ID)) {

    // If setMeasurementRate is called with a value of 0, the hardware will
    // revert to polling the hardware at the maximum rate, specified by the HAL
    // plugin.
    if (entry->endpoint == EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID) {
      emberAfCorePrintln("Relative Humidity reporting disabled");
      emberAfPluginRelativeHumidityMeasurementServerSetMeasurementRate(0);
    } else {

      //Max interval is set in seconds, which is the same unit of time the
      //emberAfPluginRelativeHumidityMeasurementServerSetMeasurementRate
      //expects in this API.
      emberAfCorePrintln("Humidity reporting interval set: %d seconds",
                          entry->data.reported.maxInterval);
      emberAfPluginRelativeHumidityMeasurementServerSetMeasurementRate(
        entry->data.reported.maxInterval);
    }
  } else if ((entry->clusterId == ZCL_ILLUM_MEASUREMENT_CLUSTER_ID)
             && (entry->attributeId == ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID)) {

    // If setMeasurementRate is called with a value of 0, the hardware will
    // revert to polling the hardware at the maximum rate, specified by the HAL
    // plugin.
    if (entry->endpoint == EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID) {
      emberAfCorePrintln("Illuminance reporting disabled");
      emberAfPluginIlluminanceMeasurementServerSetMeasurementRate(0);
    } else {

      //Max interval is set in seconds, which is the same unit of time the
      //emberAfPluginIlluminanceMeasurementServerSetMeasurementRate expects in
      //this API.
      emberAfCorePrintln("Illuminance reporting interval set: %d seconds",
                          entry->data.reported.maxInterval);
      emberAfPluginIlluminanceMeasurementServerSetMeasurementRate(
        entry->data.reported.maxInterval);
    }
  }

  return EMBER_ZCL_STATUS_SUCCESS;
}
