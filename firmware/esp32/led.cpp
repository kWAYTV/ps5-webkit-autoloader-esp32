#include <Arduino.h>
#include "led.h"

#ifdef LED_PIN
static const uint32_t LED_ACTIVITY_MS = 50;
static uint32_t ledOffUntil = 0;
#endif

void portalLedBegin()
{
#ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, !LED_ON_LEVEL);
#endif
}

void portalLedActivity()
{
#ifdef LED_PIN
    digitalWrite(LED_PIN, LED_ON_LEVEL);
    ledOffUntil = millis() + LED_ACTIVITY_MS;
#endif
}

void portalLedUpdate()
{
#ifdef LED_PIN
    if (ledOffUntil && (int32_t)(millis() - ledOffUntil) >= 0)
    {
        digitalWrite(LED_PIN, !LED_ON_LEVEL);
        ledOffUntil = 0;
    }
#endif
}
