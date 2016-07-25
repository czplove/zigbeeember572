// *****************************************************************************
// * occupancy-sensor-server.c
// *
// * Generic occupancy sensor server implementation.  This requires a HAL plugin
// * that implements the occupancy API to be included in the poject to provide
// * APIs and callbacks for initialization and device management.  It populates
// * the Occupancy and Occupancy Sensor Type attributes of the occupancy sensor
// * cluster.
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *****************************************************************************

#include "app/framework/include/af.h"
#include EMBER_AF_API_OCCUPANCY

//------------------------------------------------------------------------------
// Plugin private function prototypes
static void writeServerAttributeForAllEndpoints(EmberAfAttributeId attributeID,
                                                uint8_t* dataPtr);

//------------------------------------------------------------------------------
// Plugin consumed callback implementations

//******************************************************************************
// Plugin init function
//******************************************************************************
void emberAfPluginOccupancySensorServerInitCallback(void)
{
  HalOccupancySensorType deviceType;

  deviceType = halOccupancyGetSensorType();
  writeServerAttributeForAllEndpoints(ZCL_OCCUPANCY_SENSOR_TYPE_ATTRIBUTE_ID,
                                      (uint8_t *) &deviceType);
}

//******************************************************************************
// Notification callback from the HAL plugin
//******************************************************************************
void halOccupancyStateChangedCallback(HalOccupancyState occupancyState)
{
  if (occupancyState == HAL_OCCUPANCY_STATE_OCCUPIED) {
    emberAfOccupancySensingClusterPrintln("Occupancy detected");
  } else {
    emberAfOccupancySensingClusterPrintln("Occupancy no longer detected");
  }

  writeServerAttributeForAllEndpoints(ZCL_OCCUPANCY_ATTRIBUTE_ID,
                                      (uint8_t *) &occupancyState);
}

//------------------------------------------------------------------------------
// Plugin private functions

//******************************************************************************
// Cycle through the list of all endpoints in the system and write the given
// attribute of the occupancy sensing cluster for all endpoints in the system
// are servers of that cluster.
//******************************************************************************
static void writeServerAttributeForAllEndpoints(EmberAfAttributeId attributeID,
                                                uint8_t* dataPtr)
{
  uint8_t i;
  uint8_t endpoint;

  for (i = 0; i < emberAfEndpointCount(); i++) {
    endpoint = emberAfEndpointFromIndex(i);
    if (emberAfContainsServer(endpoint, ZCL_OCCUPANCY_SENSING_CLUSTER_ID)) {
      emberAfWriteServerAttribute(endpoint,
                                    ZCL_OCCUPANCY_SENSING_CLUSTER_ID,
                                    attributeID,
                                    dataPtr,
                                    ZCL_INT8U_ATTRIBUTE_TYPE);
    }
  }
}
