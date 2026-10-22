#include <Arduino.h>
#include "led.h"
#include "led_board.h"

#ifdef PORTAL_LED_PIN

enum LedMode : uint8_t {
    LED_BOOT,
    LED_READY,
    LED_ERROR,
};

static const uint32_t ACTIVITY_MS = 80;
static const uint32_t HEARTBEAT_ON_MS = 70;
static const uint32_t HEARTBEAT_PERIOD_MS = 1600;
static const uint32_t ERROR_MS = 120;

static LedMode mode = LED_BOOT;
static uint32_t activityUntil = 0;
static uint32_t beatStart = 0;

static void ledWrite(bool on)
{
    digitalWrite(PORTAL_LED_PIN, on ? PORTAL_LED_ON : !PORTAL_LED_ON);
}

#endif

void portalLedBegin()
{
#ifdef PORTAL_LED_PIN
    pinMode(PORTAL_LED_PIN, OUTPUT);
    mode = LED_BOOT;
    activityUntil = 0;
    ledWrite(true);
    Serial.printf("[LED] gpio=%u on=%s\n", PORTAL_LED_PIN, PORTAL_LED_ON ? "high" : "low");
#endif
}

void portalLedReady()
{
#ifdef PORTAL_LED_PIN
    mode = LED_READY;
    beatStart = millis();
    activityUntil = 0;
    ledWrite(false);
#endif
}

void portalLedActivity()
{
#ifdef PORTAL_LED_PIN
    if (mode == LED_ERROR)
        return;
    activityUntil = millis() + ACTIVITY_MS;
    ledWrite(true);
#endif
}

void portalLedError()
{
#ifdef PORTAL_LED_PIN
    mode = LED_ERROR;
    activityUntil = 0;
    beatStart = millis();
    ledWrite(true);
#endif
}

void portalLedUpdate()
{
#ifdef PORTAL_LED_PIN
    const uint32_t now = millis();

    if (activityUntil) {
        if ((int32_t)(now - activityUntil) >= 0) {
            activityUntil = 0;
            if (mode == LED_BOOT)
                ledWrite(true);
            else if (mode == LED_READY)
                ledWrite(false);
        }
        return;
    }

    if (mode == LED_BOOT) {
        ledWrite(true);
        return;
    }

    if (mode == LED_ERROR) {
        ledWrite(((now - beatStart) / ERROR_MS) & 1);
        return;
    }

    const uint32_t phase = (now - beatStart) % HEARTBEAT_PERIOD_MS;
    ledWrite(phase < HEARTBEAT_ON_MS);
#endif
}
