// *******************************************************************
// * reporting.c
// *
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/util/attribute-storage.h"
#include "reporting.h"

#ifdef ATTRIBUTE_LARGEST
#define READ_DATA_SIZE ATTRIBUTE_LARGEST
#else
#define READ_DATA_SIZE 8 // max size if attributes aren't present
#endif

#define NULL_INDEX 0xFF

static void conditionallySendReport(uint8_t endpoint, EmberAfClusterId clusterId);
static void scheduleTick(void);
static void removeConfiguration(uint8_t index);
static void removeConfigurationAndScheduleTick(uint8_t index);
static EmberAfStatus configureReceivedAttribute(const EmberAfClusterCommand *cmd,
                                                EmberAfAttributeId attributeId,
                                                uint8_t mask,
                                                uint16_t timeout);
static void putReportableChangeInResp(const EmberAfPluginReportingEntry *entry,
                                      EmberAfAttributeType dataType);
static void retrySendReport(EmberOutgoingMessageType type,
                            uint16_t indexOrDestination,
                            EmberApsFrame *apsFrame,
                            uint16_t msgLen,
                            uint8_t *message,
                            EmberStatus status);

EmberEventControl emberAfPluginReportingTickEventControl;

EmAfPluginReportVolatileData emAfPluginReportVolatileData[EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE];

static void retrySendReport(EmberOutgoingMessageType type,
                            uint16_t indexOrDestination,
                            EmberApsFrame *apsFrame,
                            uint16_t msgLen,
                            uint8_t *message,
                            EmberStatus status)
{
  // Retry once, and do so by unicasting without a pointer to this callback
  if (status != EMBER_SUCCESS) {
    emberAfSendUnicast(type, indexOrDestination, apsFrame, msgLen, message);
  }
}

#ifdef EZSP_HOST
static EmberAfPluginReportingEntry table[EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE];
void emAfPluginReportingGetEntry(uint8_t index, EmberAfPluginReportingEntry *result)
{
  MEMMOVE(result, &table[index], sizeof(EmberAfPluginReportingEntry));
}
void emAfPluginReportingSetEntry(uint8_t index, EmberAfPluginReportingEntry *value)
{
  MEMMOVE(&table[index], value, sizeof(EmberAfPluginReportingEntry));
}
#else
void emAfPluginReportingGetEntry(uint8_t index, EmberAfPluginReportingEntry *result)
{
  halCommonGetIndexedToken(result, TOKEN_REPORT_TABLE, index);
}
void emAfPluginReportingSetEntry(uint8_t index, EmberAfPluginReportingEntry *value)
{
  halCommonSetIndexedToken(TOKEN_REPORT_TABLE, index, value);
}
#endif

void emberAfPluginReportingInitCallback(void)
{
  uint8_t i;

  // On device initialization, any attributes that have been set up to report
  // should generate an attribute report.
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    EmberAfPluginReportingEntry entry;
    emAfPluginReportingGetEntry(i, &entry);
    if (entry.endpoint != EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID
        && entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED) {
      emAfPluginReportVolatileData[i].reportableChange = true;
    }
  }

  scheduleTick();
}

void emberAfPluginReportingTickEventHandler(void)
{
  EmberApsFrame *apsFrame = NULL;
  EmberAfStatus status;
  EmberAfAttributeType dataType;
  uint16_t manufacturerCode;
  uint8_t readData[READ_DATA_SIZE];
  uint8_t i, dataSize;
  bool clientToServer;
  EmberBindingTableEntry bindingEntry;
  uint8_t index, reportSize = 0, currentPayloadMaxLength = 0, smallestPayloadMaxLength;

  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    EmberAfPluginReportingEntry entry;
    uint32_t elapsedMs;
    emAfPluginReportingGetEntry(i, &entry);
    // We will only send reports for active reported attributes and only if a
    // reportable change has occurred and the minimum interval has elapsed or
    // if the maximum interval is set and has elapsed.
    elapsedMs = elapsedTimeInt32u(emAfPluginReportVolatileData[i].lastReportTimeMs,
                                  halCommonGetInt32uMillisecondTick());
    if (entry.endpoint == EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID
        || entry.direction != EMBER_ZCL_REPORTING_DIRECTION_REPORTED
        || (elapsedMs
            < entry.data.reported.minInterval * MILLISECOND_TICKS_PER_SECOND)
        || (!emAfPluginReportVolatileData[i].reportableChange
            && (entry.data.reported.maxInterval == 0
                || (elapsedMs
                    < (entry.data.reported.maxInterval
                       * MILLISECOND_TICKS_PER_SECOND))))) {
      continue;
    }

    status = emAfReadAttribute(entry.endpoint,
                               entry.clusterId,
                               entry.attributeId,
                               entry.mask,
                               entry.manufacturerCode,
                               (uint8_t *)&readData,
                               READ_DATA_SIZE,
                               &dataType);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfReportingPrintln("ERR: reading cluster 0x%2x attribute 0x%2x: 0x%x",
                              entry.clusterId,
                              entry.attributeId,
                              status);
      continue;
    }

    // find size of current report
    dataSize = (emberAfIsThisDataTypeAStringType(dataType)
                  ? emberAfStringLength(readData) + 1
                  : emberAfGetDataSize(dataType));
    reportSize = sizeof(entry.attributeId) + sizeof(dataType) + dataSize;

    // If we have already started a report for a different attribute or
    // destination, or if the current entry is too big for current report, send it and create a new one.
    if (apsFrame != NULL &&
        (!(entry.endpoint == apsFrame->sourceEndpoint
            && entry.clusterId == apsFrame->clusterId
            && emberAfClusterIsClient(&entry) == clientToServer
            && entry.manufacturerCode == manufacturerCode) ||
        (appResponseLength + reportSize > smallestPayloadMaxLength))) {
      if (appResponseLength + reportSize > smallestPayloadMaxLength) {
        emberAfReportingPrintln("Reporting Entry Full - creating new report");
      }
      conditionallySendReport(apsFrame->sourceEndpoint, apsFrame->clusterId);
      apsFrame = NULL;
    }

    // If we haven't made the message header, make it.
    if (apsFrame == NULL) {
      apsFrame = emberAfGetCommandApsFrame();
      clientToServer = emberAfClusterIsClient(&entry);
      // The manufacturer-specfic version of the fill API only creates a
      // manufacturer-specfic command if the manufacturer code is set.  For
      // non-manufacturer-specfic reports, the manufacturer code is unset, so
      // we can get away with using this API for both cases.
      emberAfFillExternalManufacturerSpecificBuffer((clientToServer
                                                     ? (ZCL_GLOBAL_COMMAND
                                                        | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                                                        | EMBER_AF_DEFAULT_RESPONSE_POLICY_REQUESTS)
                                                     : (ZCL_GLOBAL_COMMAND
                                                        | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT
                                                        | EMBER_AF_DEFAULT_RESPONSE_POLICY_REQUESTS)),
                                                    entry.clusterId,
                                                    entry.manufacturerCode,
                                                    ZCL_REPORT_ATTRIBUTES_COMMAND_ID,
                                                    "");
      apsFrame->sourceEndpoint = entry.endpoint;
      apsFrame->options = EMBER_AF_DEFAULT_APS_OPTIONS;
      manufacturerCode = entry.manufacturerCode;

      // EMAPPFWKV2-1327: Reporting plugin does not account for reporting too many attributes
      //                  in the same ZCL:ReportAttributes message

      // find smallest maximum payload that the destination can receive for this cluster and source endpoint
      smallestPayloadMaxLength = MAX_INT8U_VALUE;
      for (index = 0; index < EMBER_BINDING_TABLE_SIZE; index++) {
        status = (EmberAfStatus)emberGetBinding(index, &bindingEntry);
        if (status == (EmberAfStatus)EMBER_SUCCESS && bindingEntry.local == entry.endpoint && bindingEntry.clusterId == entry.clusterId) {
          currentPayloadMaxLength = emberAfMaximumApsPayloadLength(bindingEntry.type, bindingEntry.networkIndex, apsFrame);
          if (currentPayloadMaxLength < smallestPayloadMaxLength) {
            smallestPayloadMaxLength = currentPayloadMaxLength;
          }
        }
      }

    }

    // Payload is [attribute id:2] [type:1] [data:N].
    emberAfPutInt16uInResp(entry.attributeId);
    emberAfPutInt8uInResp(dataType);

#if (BIGENDIAN_CPU)
    if (isThisDataTypeSentLittleEndianOTA(dataType)) {
      uint8_t i;
      for (i = 0; i < dataSize; i++) {
        emberAfPutInt8uInResp(readData[dataSize - i - 1]);
      }
    } else {
      emberAfPutBlockInResp(readData, dataSize);
    }
#else
    emberAfPutBlockInResp(readData, dataSize);
#endif

    // Store the last reported time and value so that we can track intervals
    // and changes.  We only track changes for data types that are small enough
    // for us to compare.
    emAfPluginReportVolatileData[i].reportableChange = false;
    emAfPluginReportVolatileData[i].lastReportTimeMs = halCommonGetInt32uMillisecondTick();
    if (dataSize <= sizeof(emAfPluginReportVolatileData[i].lastReportValue)) {
      emAfPluginReportVolatileData[i].lastReportValue = 0;
#if (BIGENDIAN_CPU)
      MEMMOVE(((uint8_t *)&emAfPluginReportVolatileData[i].lastReportValue
               + sizeof(emAfPluginReportVolatileData[i].lastReportValue)
               - dataSize),
              readData,
              dataSize);
#else
      MEMMOVE(&emAfPluginReportVolatileData[i].lastReportValue, readData, dataSize);
#endif
    }
  }

  if (apsFrame != NULL) {
    conditionallySendReport(apsFrame->sourceEndpoint, apsFrame->clusterId);
  }
  scheduleTick();
}

static void conditionallySendReport(uint8_t endpoint, EmberAfClusterId clusterId)
{
  EmberStatus status;
  if (emberAfIsDeviceEnabled(endpoint)
      || clusterId == ZCL_IDENTIFY_CLUSTER_ID) {
    status = emberAfSendCommandUnicastToBindingsWithCallback((EmberAfMessageSentFunction)(&retrySendReport));

    // If the callback table is full, attempt to send the message with no
    // callback.  Note that this could lead to a message failing to transmit
    // with no notification to the user for any number of reasons (ex: hitting
    // the message queue limit), but is better than not sending the message at
    // all because the system hits its callback queue limit.
    if (status == EMBER_TABLE_FULL) {
      emberAfSendCommandUnicastToBindings();
    }
  }
}

bool emberAfConfigureReportingCommandCallback(const EmberAfClusterCommand *cmd)
{
  uint16_t bufIndex = cmd->payloadStartIndex;
  uint8_t frameControl, mask;
  bool failures = false;

  emberAfReportingPrint("%p: ", "CFG_RPT");
  emberAfReportingDebugExec(emberAfDecodeAndPrintCluster(cmd->apsFrame->clusterId));
  emberAfReportingPrintln("");
  emberAfReportingFlush();

  if (cmd->direction == ZCL_DIRECTION_CLIENT_TO_SERVER) {
    frameControl = (ZCL_GLOBAL_COMMAND
                    | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT
                    | EMBER_AF_DEFAULT_RESPONSE_POLICY_REQUESTS);
    mask = CLUSTER_MASK_SERVER;
  } else {
    frameControl = (ZCL_GLOBAL_COMMAND
                    | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                    | EMBER_AF_DEFAULT_RESPONSE_POLICY_REQUESTS);
    mask = CLUSTER_MASK_CLIENT;
  }

  // The manufacturer-specfic version of the fill API only creates a
  // manufacturer-specfic command if the manufacturer code is set.  For non-
  // manufacturer-specfic reports, the manufacturer code is unset, so we can
  // get away with using this API for both cases.
  emberAfFillExternalManufacturerSpecificBuffer(frameControl,
                                                cmd->apsFrame->clusterId,
                                                cmd->mfgCode,
                                                ZCL_CONFIGURE_REPORTING_RESPONSE_COMMAND_ID,
                                                "");

  // Each record in the command has at least a one-byte direction and a two-
  // byte attribute id.  Additional fields are present depending on the value
  // of the direction field.
  while (bufIndex + 3 < cmd->bufLen) {
    EmberAfAttributeId attributeId;
    EmberAfReportingDirection direction;
    EmberAfStatus status;

    direction = (EmberAfReportingDirection)emberAfGetInt8u(cmd->buffer,
                                                           bufIndex,
                                                           cmd->bufLen);
    bufIndex++;
    attributeId = (EmberAfAttributeId)emberAfGetInt16u(cmd->buffer,
                                                       bufIndex,
                                                       cmd->bufLen);
    bufIndex += 2;

    emberAfReportingPrintln(" - direction:%x, attr:%2x", direction, attributeId);

    switch (direction) {
    case EMBER_ZCL_REPORTING_DIRECTION_REPORTED:
      {
        EmberAfAttributeMetadata* metadata;
        EmberAfAttributeType dataType;
        uint16_t minInterval, maxInterval;
        uint32_t reportableChange = 0;
        EmberAfPluginReportingEntry newEntry;

        dataType = (EmberAfAttributeType)emberAfGetInt8u(cmd->buffer,
                                                         bufIndex,
                                                         cmd->bufLen);
        bufIndex++;
        minInterval = emberAfGetInt16u(cmd->buffer, bufIndex, cmd->bufLen);
        bufIndex += 2;
        maxInterval = emberAfGetInt16u(cmd->buffer, bufIndex, cmd->bufLen);
        bufIndex += 2;

        emberAfReportingPrintln("   type:%x, min:%2x, max:%2x",
                                dataType,
                                minInterval,
                                maxInterval);
        emberAfReportingFlush();

        if (emberAfGetAttributeAnalogOrDiscreteType(dataType)
            == EMBER_AF_DATA_TYPE_ANALOG) {
          uint8_t dataSize = emberAfGetDataSize(dataType);
          reportableChange = emberAfGetInt(cmd->buffer,
                                           bufIndex,
                                           cmd->bufLen,
                                           dataSize);

          emberAfReportingPrint("   change:");
          emberAfReportingPrintBuffer(cmd->buffer + bufIndex, dataSize, false);
          emberAfReportingPrintln("");

          bufIndex += dataSize;
        }

        // emberAfPluginReportingConfigureReportedAttribute handles non-
        // existent attributes, but does not verify the attribute data type, so
        // we need to check it here.
        metadata = emberAfLocateAttributeMetadata(cmd->apsFrame->destinationEndpoint,
                                                  cmd->apsFrame->clusterId,
                                                  attributeId,
                                                  mask,
                                                  cmd->mfgCode);
        if (metadata != NULL && metadata->attributeType != dataType) {
          status = EMBER_ZCL_STATUS_INVALID_DATA_TYPE;
        } else {
          // Add a reporting entry for a reported attribute.  The reports will
          // be sent from us to the source of the Configure Reporting command.
          newEntry.endpoint = cmd->apsFrame->destinationEndpoint;
          newEntry.clusterId = cmd->apsFrame->clusterId;
          newEntry.attributeId = attributeId;
          newEntry.mask = mask;
          newEntry.manufacturerCode = cmd->mfgCode;
          newEntry.data.reported.minInterval = minInterval;
          newEntry.data.reported.maxInterval = maxInterval;
          newEntry.data.reported.reportableChange = reportableChange;
          status = emberAfPluginReportingConfigureReportedAttribute(&newEntry);
        }
        break;
      }
    case EMBER_ZCL_REPORTING_DIRECTION_RECEIVED:
      {
        uint16_t timeout = emberAfGetInt16u(cmd->buffer, bufIndex, cmd->bufLen);
        bufIndex += 2;

        emberAfReportingPrintln("   timeout:%2x", timeout);

        // Add a reporting entry from a received attribute.  The reports
        // will be sent to us from the source of the Configure Reporting
        // command.
        status = configureReceivedAttribute(cmd,
                                            attributeId,
                                            mask,
                                            timeout);
        break;
      }
    default:
      // This will abort the processing (see below).
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      break;
    }

    // If a report cannot be configured, the status, direction, and
    // attribute are added to the response.  If the failure was due to an
    // invalid field, we have to abort after this record because we don't
    // know how to interpret the rest of the data in the request.
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPutInt8uInResp(status);
      emberAfPutInt8uInResp(direction);
      emberAfPutInt16uInResp(attributeId);
      failures = true;
      if (status == EMBER_ZCL_STATUS_INVALID_FIELD) {
        break;
      }
    }
  }

  // We just respond with SUCCESS if we made it through without failures.
  if (!failures) {
    emberAfPutInt8uInResp(EMBER_ZCL_STATUS_SUCCESS);
  }

  emberAfSendResponse();
  return true;
}

bool emberAfReadReportingConfigurationCommandCallback(const EmberAfClusterCommand *cmd)
{
  uint16_t bufIndex = cmd->payloadStartIndex;
  uint8_t frameControl, mask;

  emberAfReportingPrint("%p: ", "READ_RPT_CFG");
  emberAfReportingDebugExec(emberAfDecodeAndPrintCluster(cmd->apsFrame->clusterId));
  emberAfReportingPrintln("");
  emberAfReportingFlush();

  if (cmd->direction == ZCL_DIRECTION_CLIENT_TO_SERVER) {
    frameControl = (ZCL_GLOBAL_COMMAND
                    | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT
                    | EMBER_AF_DEFAULT_RESPONSE_POLICY_REQUESTS);
    mask = CLUSTER_MASK_SERVER;
  } else {
    frameControl = (ZCL_GLOBAL_COMMAND
                    | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                    | EMBER_AF_DEFAULT_RESPONSE_POLICY_REQUESTS);
    mask = CLUSTER_MASK_CLIENT;
  }

  // The manufacturer-specfic version of the fill API only creates a
  // manufacturer-specfic command if the manufacturer code is set.  For non-
  // manufacturer-specfic reports, the manufacturer code is unset, so we can
  // get away with using this API for both cases.
  emberAfFillExternalManufacturerSpecificBuffer(frameControl,
                                                cmd->apsFrame->clusterId,
                                                cmd->mfgCode,
                                                ZCL_READ_REPORTING_CONFIGURATION_RESPONSE_COMMAND_ID,
                                                "");

  // Each record in the command has a one-byte direction and a two-byte
  // attribute id.
  while (bufIndex + 3 <= cmd->bufLen) {
    EmberAfAttributeId attributeId;
    EmberAfAttributeMetadata *metadata;
    EmberAfPluginReportingEntry entry;
    EmberAfReportingDirection direction;
    uint8_t i;
    bool found = false;

    direction = (EmberAfReportingDirection)emberAfGetInt8u(cmd->buffer,
                                                           bufIndex,
                                                           cmd->bufLen);
    bufIndex++;
    attributeId = (EmberAfAttributeId)emberAfGetInt16u(cmd->buffer,
                                                       bufIndex,
                                                       cmd->bufLen);
    bufIndex += 2;

    switch (direction) {
    case EMBER_ZCL_REPORTING_DIRECTION_REPORTED:
      metadata = emberAfLocateAttributeMetadata(cmd->apsFrame->destinationEndpoint,
                                                cmd->apsFrame->clusterId,
                                                attributeId,
                                                mask,
                                                cmd->mfgCode);
      if (metadata == NULL) {
        emberAfPutInt8uInResp(EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE);
        emberAfPutInt8uInResp(direction);
        emberAfPutInt16uInResp(attributeId);
        continue;
      }
      break;
    case EMBER_ZCL_REPORTING_DIRECTION_RECEIVED:
      break;
    default:
      emberAfPutInt8uInResp(EMBER_ZCL_STATUS_INVALID_FIELD);
      emberAfPutInt8uInResp(direction);
      emberAfPutInt16uInResp(attributeId);
      continue;
    }

    // 075123r03 seems to suggest that SUCCESS is returned even if reporting
    // isn't configured for the requested attribute.  The individual fields
    // of the response for this attribute get populated with defaults.
    // CCB 1854 removes the ambiguity and requires NOT_FOUND to be returned in
    // the status field and all fields except direction and attribute identifier
    // to be omitted if there is no report configuration found.
    for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
      emAfPluginReportingGetEntry(i, &entry);
      if (entry.direction == direction
          && entry.endpoint == cmd->apsFrame->destinationEndpoint
          && entry.clusterId == cmd->apsFrame->clusterId
          && entry.attributeId == attributeId
          && entry.mask == mask
          && entry.manufacturerCode == cmd->mfgCode
          && (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED
              || (entry.data.received.source == cmd->source
                  && entry.data.received.endpoint == cmd->apsFrame->sourceEndpoint))) {
        found = true;
        break;
      }
    }
    // Attribute supported, reportable, no report configuration was found.
    if (found == false) {
      emberAfPutInt8uInResp(EMBER_ZCL_STATUS_NOT_FOUND);
      emberAfPutInt8uInResp(direction);
      emberAfPutInt16uInResp(attributeId);
      continue;
    }
    // Attribute supported, reportable, report configuration was found.
    emberAfPutInt8uInResp(EMBER_ZCL_STATUS_SUCCESS);
    emberAfPutInt8uInResp(direction);
    emberAfPutInt16uInResp(attributeId);
    switch (direction) {
      case EMBER_ZCL_REPORTING_DIRECTION_REPORTED:
        emberAfPutInt8uInResp(metadata->attributeType);
        emberAfPutInt16uInResp(entry.data.reported.minInterval);
        emberAfPutInt16uInResp(entry.data.reported.maxInterval);
        if (emberAfGetAttributeAnalogOrDiscreteType(metadata->attributeType)
            == EMBER_AF_DATA_TYPE_ANALOG) {
          putReportableChangeInResp(&entry, metadata->attributeType);
        }
        break;
      case EMBER_ZCL_REPORTING_DIRECTION_RECEIVED:
        emberAfPutInt16uInResp(entry.data.received.timeout);
        break;
    }
  }

  emberAfSendResponse();
  return true;
}

EmberStatus emberAfClearReportTableCallback(void)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    removeConfiguration(i);
  }
  emberEventControlSetInactive(emberAfPluginReportingTickEventControl);
  return EMBER_SUCCESS;
}

EmberStatus emAfPluginReportingRemoveEntry(uint8_t index)
{
  EmberStatus status = EMBER_INDEX_OUT_OF_RANGE;
  if (index < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE) {
    removeConfigurationAndScheduleTick(index);
    status = EMBER_SUCCESS;
  }
  return status;
}

void emberAfReportingAttributeChangeCallback(uint8_t endpoint,
                                             EmberAfClusterId clusterId,
                                             EmberAfAttributeId attributeId,
                                             uint8_t mask,
                                             uint16_t manufacturerCode,
                                             EmberAfAttributeType type,
                                             uint8_t *data)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    EmberAfPluginReportingEntry entry;
    emAfPluginReportingGetEntry(i, &entry);
    if (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED
        && entry.endpoint == endpoint
        && entry.clusterId == clusterId
        && entry.attributeId == attributeId
        && entry.mask == mask
        && entry.manufacturerCode == manufacturerCode) {
      // If we are reporting this particular attribute, we only care whether
      // the new value meets the reportable change criteria.  If it does, we
      // mark the entry as ready to report and reschedule the tick.  Whether
      // the tick will be scheduled for immediate or delayed execution depends
      // on the minimum reporting interval.  This is handled in the scheduler.
      uint32_t difference = emberAfGetDifference(data,
                                               emAfPluginReportVolatileData[i].lastReportValue,
                                               emberAfGetDataSize(type));
      uint8_t analogOrDiscrete = emberAfGetAttributeAnalogOrDiscreteType(type);
      if ((analogOrDiscrete == EMBER_AF_DATA_TYPE_DISCRETE && difference != 0)
          || (analogOrDiscrete == EMBER_AF_DATA_TYPE_ANALOG
              && entry.data.reported.reportableChange <= difference)) {
        emAfPluginReportVolatileData[i].reportableChange = true;
        scheduleTick();
      }
      break;
    }
  }
}

static void scheduleTick(void)
{
  uint32_t delayMs = MAX_INT32U_VALUE;
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    EmberAfPluginReportingEntry entry;
    emAfPluginReportingGetEntry(i, &entry);
    if (entry.endpoint != EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID
        && entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED) {
      uint32_t minIntervalMs = (entry.data.reported.minInterval
                                * MILLISECOND_TICKS_PER_SECOND);
      uint32_t maxIntervalMs = (entry.data.reported.maxInterval
                                * MILLISECOND_TICKS_PER_SECOND);
      uint32_t elapsedMs = elapsedTimeInt32u(emAfPluginReportVolatileData[i].lastReportTimeMs,
                                           halCommonGetInt32uMillisecondTick());
      uint32_t remainingMs = MAX_INT32U_VALUE;
      if (emAfPluginReportVolatileData[i].reportableChange) {
        remainingMs = (minIntervalMs < elapsedMs
                       ? 0
                       : minIntervalMs - elapsedMs);
      } else if (maxIntervalMs) {
        remainingMs = (maxIntervalMs < elapsedMs
                       ? 0
                       : maxIntervalMs - elapsedMs);
      }
      if (remainingMs < delayMs) {
        delayMs = remainingMs;
      }
    }
  }
  if (delayMs != MAX_INT32U_VALUE) {
    emberAfDebugPrintln("sched report event for: 0x%4x", delayMs);
    emberAfEventControlSetDelayMS(&emberAfPluginReportingTickEventControl,
                                  delayMs);
  } else {
    emberAfDebugPrintln("deactivate report event");
    emberEventControlSetInactive(emberAfPluginReportingTickEventControl);
  }
}

static void removeConfiguration(uint8_t index)
{
  EmberAfPluginReportingEntry entry;
  emAfPluginReportingGetEntry(index, &entry);
  entry.endpoint = EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID;
  emAfPluginReportingSetEntry(index, &entry);
  emberAfPluginReportingConfiguredCallback(&entry);
}

static void removeConfigurationAndScheduleTick(uint8_t index)
{
  removeConfiguration(index);
  scheduleTick();
}

EmberAfStatus emberAfPluginReportingConfigureReportedAttribute(const EmberAfPluginReportingEntry* newEntry)
{
  EmberAfAttributeMetadata *metadata;
  EmberAfPluginReportingEntry entry;
  EmberAfStatus status;
  uint8_t i, index = NULL_INDEX;
  bool initialize = true;

  // Verify that we support the attribute and that the data type matches.
  metadata = emberAfLocateAttributeMetadata(newEntry->endpoint,
                                            newEntry->clusterId,
                                            newEntry->attributeId,
                                            newEntry->mask,
                                            newEntry->manufacturerCode);
  if (metadata == NULL) {
    return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
  }

  // Verify the minimum and maximum intervals make sense.
  if (newEntry->data.reported.maxInterval != 0
      && (newEntry->data.reported.maxInterval
          < newEntry->data.reported.minInterval)) {
    return EMBER_ZCL_STATUS_INVALID_VALUE;
  }

  // Check the table for an entry that matches this request and also watch for
  // empty slots along the way.  If a report exists, it will be overwritten
  // with the new configuration.  Otherwise, a new entry will be created and
  // initialized.
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    emAfPluginReportingGetEntry(i, &entry);
    if (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED
        && entry.endpoint == newEntry->endpoint
        && entry.clusterId == newEntry->clusterId
        && entry.attributeId == newEntry->attributeId
        && entry.mask == newEntry->mask
        && entry.manufacturerCode == newEntry->manufacturerCode) {
      initialize = false;
      index = i;
      break;
    } else if (entry.endpoint == EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID
               && index == NULL_INDEX) {
      index = i;
    }
  }

  // If the maximum reporting interval is 0xFFFF, the device shall not issue
  // reports for the attribute and the configuration information for that
  // attribute need not be maintained.
  if (newEntry->data.reported.maxInterval == 0xFFFF) {
    if (!initialize) {
      removeConfigurationAndScheduleTick(index);
    }
    return EMBER_ZCL_STATUS_SUCCESS;
  }

  if (index == NULL_INDEX) {
    return EMBER_ZCL_STATUS_INSUFFICIENT_SPACE;
  } else if (initialize) {
    entry.direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
    entry.endpoint = newEntry->endpoint;
    entry.clusterId = newEntry->clusterId;
    entry.attributeId = newEntry->attributeId;
    entry.mask = newEntry->mask;
    entry.manufacturerCode = newEntry->manufacturerCode;
    emAfPluginReportVolatileData[index].lastReportTimeMs = halCommonGetInt32uMillisecondTick();
    emAfPluginReportVolatileData[index].lastReportValue = 0;
  }

  // For new or updated entries, set the intervals and reportable change.
  // Updated entries will retain all other settings configured previously.
  entry.data.reported.minInterval = newEntry->data.reported.minInterval;
  entry.data.reported.maxInterval = newEntry->data.reported.maxInterval;
  entry.data.reported.reportableChange = newEntry->data.reported.reportableChange;

  // Give the application a chance to review the configuration that we have
  // been building up.  If the application rejects it, we just do not save the
  // record.  If we were supposed to add a new configuration, it will not be
  // created.  If we were supposed to update an existing configuration, we will
  // keep the old one and just discard any changes.  So, in either case, life
  // continues unchanged if the application rejects the configuration.
  status = emberAfPluginReportingConfiguredCallback(&entry);
  if (status == EMBER_ZCL_STATUS_SUCCESS) {
    emAfPluginReportingSetEntry(index, &entry);
    scheduleTick();
  }
  return status;
}

static EmberAfStatus configureReceivedAttribute(const EmberAfClusterCommand *cmd,
                                                EmberAfAttributeId attributeId,
                                                uint8_t mask,
                                                uint16_t timeout)
{
  EmberAfPluginReportingEntry entry;
  EmberAfStatus status;
  uint8_t i, index = NULL_INDEX;
  bool initialize = true;

  // Check the table for an entry that matches this request and also watch for
  // empty slots along the way.  If a report exists, it will be overwritten
  // with the new configuration.  Otherwise, a new entry will be created and
  // initialized.
  for (i = 0; i < EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE; i++) {
    emAfPluginReportingGetEntry(i, &entry);
    if (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_RECEIVED
        && entry.endpoint == cmd->apsFrame->destinationEndpoint
        && entry.clusterId == cmd->apsFrame->clusterId
        && entry.attributeId == attributeId
        && entry.mask == mask
        && entry.manufacturerCode == cmd->mfgCode
        && entry.data.received.source == cmd->source
        && entry.data.received.endpoint == cmd->apsFrame->sourceEndpoint) {
      initialize = false;
      index = i;
      break;
    } else if (entry.endpoint == EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID
               && index == NULL_INDEX) {
      index = i;
    }
  }

  if (index == NULL_INDEX) {
    return EMBER_ZCL_STATUS_INSUFFICIENT_SPACE;
  } else if (initialize) {
    entry.direction = EMBER_ZCL_REPORTING_DIRECTION_RECEIVED;
    entry.endpoint = cmd->apsFrame->destinationEndpoint;
    entry.clusterId = cmd->apsFrame->clusterId;
    entry.attributeId = attributeId;
    entry.mask = mask;
    entry.manufacturerCode = cmd->mfgCode;
    entry.data.received.source = cmd->source;
    entry.data.received.endpoint = cmd->apsFrame->sourceEndpoint;
  }

  // For new or updated entries, set the timeout.  Updated entries will retain
  // all other settings configured previously.
  entry.data.received.timeout = timeout;

  // Give the application a chance to review the configuration that we have
  // been building up.  If the application rejects it, we just do not save the
  // record.  If we were supposed to add a new configuration, it will not be
  // created.  If we were supposed to update an existing configuration, we will
  // keep the old one and just discard any changes.  So, in either case, life
  // continues unchanged if the application rejects the configuration.  If the
  // application accepts the change, the tick does not have to be rescheduled
  // here because we don't do anything with received reports.
  status = emberAfPluginReportingConfiguredCallback(&entry);
  if (status == EMBER_ZCL_STATUS_SUCCESS) {
    emAfPluginReportingSetEntry(index, &entry);
  }
  return status;
}

static void putReportableChangeInResp(const EmberAfPluginReportingEntry *entry,
                                      EmberAfAttributeType dataType)
{
  uint8_t bytes = emberAfGetDataSize(dataType);
  if (entry == NULL) { // default, 0xFF...UL or 0x80...L
    for (; bytes > 0; bytes--) {
      uint8_t b = 0xFF;
      if (emberAfIsTypeSigned(dataType)) {
        b = (bytes == 1 ? 0x80 : 0x00);
      }
      emberAfPutInt8uInResp(b);
    }
  } else { // reportable change value
    uint32_t value = entry->data.reported.reportableChange;
    for (; bytes > 0; bytes--) {
      uint8_t b = BYTE_0(value);
      emberAfPutInt8uInResp(b);
      value >>= 8;
    }
  }
}
