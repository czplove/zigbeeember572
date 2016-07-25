// Copyright 2007 - 2012 by Ember Corporation. All rights reserved.
//
//

#include "app/framework/include/af.h"
#include "app/framework/plugin/counters/counters.h"
#include "diagnostic-server.h"
#include "app/framework/util/attribute-storage.h"
#include "app/util/common/common.h"

bool emberAfReadDiagnosticAttribute(
                    EmberAfAttributeMetadata *attributeMetadata, 
                    uint8_t *buffer) {
  uint8_t emberCounter = EMBER_COUNTER_TYPE_COUNT;
  EmberStatus status;

  switch (attributeMetadata->attributeId) {
    case ZCL_MAC_RX_BCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_MAC_RX_BROADCAST;
      break;
    case ZCL_MAC_TX_BCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_MAC_TX_BROADCAST;
      break;
    case ZCL_MAC_RX_UCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_MAC_RX_UNICAST;
      break;
    case ZCL_MAC_TX_UCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_MAC_TX_UNICAST_SUCCESS;
      break;
    case ZCL_MAC_TX_UCAST_RETRY_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_MAC_TX_UNICAST_RETRY;
      break;
    case ZCL_MAC_TX_UCAST_FAIL_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_MAC_TX_UNICAST_FAILED;
      break;
    case ZCL_APS_RX_BCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DATA_RX_BROADCAST;
      break;
    case ZCL_APS_TX_BCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DATA_TX_BROADCAST;
      break;
    case ZCL_APS_RX_UCAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DATA_RX_UNICAST;
      break;
    case ZCL_APS_UCAST_SUCCESS_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS;
      break;
    case ZCL_APS_TX_UCAST_RETRY_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DATA_TX_UNICAST_RETRY;
      break;
    case ZCL_APS_TX_UCAST_FAIL_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED;
      break;
    case ZCL_ROUTE_DISC_INITIATED_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_ROUTE_DISCOVERY_INITIATED;
      break;
    case ZCL_NEIGHBOR_ADDED_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_NEIGHBOR_ADDED;
      break;
    case ZCL_NEIGHBOR_REMOVED_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_NEIGHBOR_REMOVED;
      break;
    case ZCL_NEIGHBOR_STALE_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_NEIGHBOR_STALE;
      break;
    case ZCL_JOIN_INDICATION_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_JOIN_INDICATION;
      break;
    case ZCL_CHILD_MOVED_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_CHILD_REMOVED;
      break;
    case ZCL_NWK_FC_FAILURE_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_NWK_FRAME_COUNTER_FAILURE;
      break;
    case ZCL_APS_FC_FAILURE_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_FRAME_COUNTER_FAILURE;
      break;
    case ZCL_APS_UNAUTHORIZED_KEY_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_LINK_KEY_NOT_AUTHORIZED;
      break;
    case ZCL_NWK_DECRYPT_FAILURE_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_NWK_DECRYPTION_FAILURE;
      break;
    case ZCL_APS_DECRYPT_FAILURE_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_APS_DECRYPTION_FAILURE;
      break;
    case ZCL_PACKET_BUFFER_ALLOC_FAILURES_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_ALLOCATE_PACKET_BUFFER_FAILURE;
      break;
    case ZCL_RELAYED_UNICAST_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_RELAYED_UNICAST;
      break;
    case ZCL_PHY_TO_MAC_QUEUE_LIMIT_REACHED_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_PHY_TO_MAC_QUEUE_LIMIT_REACHED;
      break;
    case ZCL_PACKET_VALIDATE_DROP_COUNT_ATTRIBUTE_ID:
      emberCounter = EMBER_COUNTER_PACKET_VALIDATE_LIBRARY_DROPPED_COUNT;
      break;
    default:
      break;
  }
  if (emberCounter < EMBER_COUNTER_TYPE_COUNT) {
    // The emberCounters array is 16-bit values, so we only have two bytes to
    // return to the caller, regardless of how big the buffer is.
    MEMSET(buffer, 0x00, attributeMetadata->size);
    buffer[0] = LOW_BYTE(emberCounters[emberCounter]);
    buffer[1] = HIGH_BYTE(emberCounters[emberCounter]);
    return true;
  }
  // code for handling diagnostic attributes that need to be computed
  switch (attributeMetadata->attributeId) {
    case ZCL_NUMBER_OF_RESETS_ATTRIBUTE_ID:
      {
        tokTypeStackBootCounter rebootCounter;

        uint16_t rebootCounter16;

        halCommonGetToken(&rebootCounter, TOKEN_STACK_BOOT_COUNTER);

        // in case we are using simee2 and the reboot counter is an uint32_t.
        rebootCounter16 = (uint16_t) rebootCounter;

        MEMMOVE(buffer, &rebootCounter16, 2);

        return true;
      }
      break;
    case ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID:
      {
        uint32_t scratch;
        uint16_t macRetriesPerAps;

        scratch = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS] +
                  emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED];
        if(scratch > 0) {
          scratch = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]
                    / scratch;
        }

        macRetriesPerAps = (uint16_t) scratch;
        MEMMOVE(buffer, &macRetriesPerAps, 2);
        return true;
      }
      break;
    case ZCL_LAST_MESSAGE_LQI_ATTRIBUTE_ID:
      status = emberGetLastHopLqi( buffer );
      assert(status == EMBER_ZCL_STATUS_SUCCESS);

      emberSerialPrintf(APP_SERIAL, "LQI:  %x %x\r\n", buffer[0]);

      return true;
      break;
    case ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID:
      {
        int8_t rssi;
        status = emberGetLastHopRssi( &rssi );
        assert(status == EMBER_ZCL_STATUS_SUCCESS);

        buffer[0] = (uint8_t) rssi;

        emberSerialPrintf(APP_SERIAL, "RSSI:  %x %x\r\n", buffer[0]);

        return true;
      }
      break;
    default:
      break;
  }

  return false;
}

/** @brief External Attribute Read
 *
 * Like emberAfExternalAttributeWriteCallback above, this function is called
 * when the framework needs to read an attribute that is not stored within the
 * Application Framework's data structures.
        All of the important
 * information about the attribute itself is passed as a pointer to an
 * EmberAfAttributeMetadata struct, which is stored within the application and
 * used to manage the attribute. A complete description of the
 * EmberAfAttributeMetadata struct is provided in
 * app/framework/include/af-types.h
        This function assumes that the
 * application is able to read the attribute, write it into the passed buffer,
 * and return immediately. Any attributes that require a state machine for
 * reading and writing are not really candidates for externalization at the
 * present time. The Application Framework does not currently include a state
 * machine for reading or writing attributes that must take place across a
 * series of application ticks. Attributes that cannot be read in a timely
 * manner should be stored within the Application Framework and updated
 * occasionally by the application code from within the
 * emberAfMainTickCallback.
        If the application was successfully able
 * to read the attribute and write it into the passed buffer, it should return
 * a value of EMBER_ZCL_STATUS_SUCCESS. Any other return value indicates the
 * application was not able to read the attribute.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeMetadata   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param buffer   Ver.: always
 */
EmberAfStatus emberAfStackDiagnosticAttributeReadCallback(uint8_t endpoint,
                                                   EmberAfClusterId clusterId,
                                                   EmberAfAttributeMetadata * attributeMetadata,
                                                   uint16_t manufacturerCode,
                                                   uint8_t * buffer)
{
  if(emberAfReadDiagnosticAttribute(attributeMetadata, buffer)) {
    return EMBER_ZCL_STATUS_SUCCESS;
  } else {
    return EMBER_ZCL_STATUS_FAILURE;
  }
}

bool emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage)
{
  uint8_t data;
  uint16_t macRetriesPerAps;

  uint8_t fixedEndpoints[] = FIXED_ENDPOINT_ARRAY;

  uint8_t endpoint = fixedEndpoints[0];

  // grab last hop LQI and RSSI here.  write them to the framework.
  emberGetLastHopLqi(&data);
  emberAfWriteServerAttribute(endpoint,
                              ZCL_DIAGNOSTICS_CLUSTER_ID,        //0x0b05
                              ZCL_LAST_MESSAGE_LQI_ATTRIBUTE_ID, // 0x011c
                              &data,
                              ZCL_INT8U_ATTRIBUTE_TYPE);         //0x20
  emberGetLastHopRssi( (int8_t *) &data );
  emberAfWriteServerAttribute(endpoint,
                              ZCL_DIAGNOSTICS_CLUSTER_ID,         // 0x0b05
                              ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID, // 0x011d
                              &data,
                              ZCL_INT8S_ATTRIBUTE_TYPE);          // 0x28
  {
    uint32_t scratch;

    scratch = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS] +
                emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED];
    if(scratch > 0) {
      scratch = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]
                / scratch;
    }

    macRetriesPerAps = (uint16_t) scratch;
    emberAfWriteServerAttribute(endpoint,
                                ZCL_DIAGNOSTICS_CLUSTER_ID,         // 0x0b05
                                ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID, // 0x011b
                                (uint8_t *) &macRetriesPerAps,
                                ZCL_INT16U_ATTRIBUTE_TYPE);          // 0x21
  }

  return false;
}
