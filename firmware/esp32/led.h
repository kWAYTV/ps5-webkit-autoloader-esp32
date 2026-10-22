#pragma once

#ifdef LED_PIN
#ifndef LED_ON_LEVEL
#define LED_ON_LEVEL LOW
#endif
#endif

void portalLedBegin();
void portalLedActivity();
void portalLedUpdate();
