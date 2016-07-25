//   Copyright 2015 Silicon Laboratories, Inc.                              *80*

#include PLATFORM_HEADER
#include "stack/include/ember-types.h"

EmberEventControl emberAfPluginLedBlinkLed0EventFunctionEventControl;
EmberEventControl emberAfPluginLedBlinkLed1EventFunctionEventControl;

void emberAfPluginLedBlinkLed0EventFunctionEventHandler(void)
{
}

void emberAfPluginLedBlinkLed1EventFunctionEventHandler(void)
{
}

void halLedBlinkSetActivityLed(uint8_t led)
{
}

void halLedBlinkBlink(uint8_t count, uint16_t blinkTime)
{
}

void halLedBlinkPattern(uint8_t count, uint8_t length, uint16_t *pattern)
{
}

void halLedBlinkLedOff(uint8_t time)
{
}

void halLedBlinkLedOn(uint8_t time)
{
}

void halLedBlinkSleepyClearGpio(uint8_t port, uint8_t pin)
{
}

void halLedBlinkSleepySetGpio(uint8_t port, uint8_t pin)
{
}
