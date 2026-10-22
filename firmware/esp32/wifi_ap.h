#pragma once

#include <Arduino.h>

bool portalDnsAllowed(const String &domain);
void portalStartAccessPoint();
void portalStartDns();
void portalProcessDns();
