// Copyright 2015 Silicon Laboratories, Inc.                                *80*

#ifndef __SILABS_DEVICE_UI_H__
#define __SILABS_DEVICE_UI_H__

//------------------------------------------------------------------------------
// Plugin public function declarations

/** @brief Blink the Network Found LED pattern
 */
void emberAfPluginSilabsDeviceUiLedNetworkFoundBlink(void);

/** @brief Blink the Network Lost LED pattern
 */
void emberAfPluginSilabsDeviceUiLedNetworkLostBlink(void);

/** @brief Blink the Network Searching LED pattern
 */
void emberAfPluginSilabsDeviceUiLedNetworkSearchingBlink(void);

/** @brief Blink the Network Identify LED pattern
 */
void emberAfPluginSilabsDeviceUiLedIdentifyBlink(void);

/** @brief Blink the Network Proactive Rejoin LED pattern
 */
void emberAfPluginSilabsDeviceUiLedProactiveRejoinBlink(void);

#endif //__SILABS_DEVICE_UI_H__
