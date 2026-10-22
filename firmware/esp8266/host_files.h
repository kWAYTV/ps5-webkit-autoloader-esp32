#pragma once

#include <Arduino.h>
#include <LittleFS.h>

String portalMimeType(const String &path);
String portalNormalizePath(const String &uri);
bool portalRewriteMirrorPath(String &path);
void portalLogHeap(const char *where);
