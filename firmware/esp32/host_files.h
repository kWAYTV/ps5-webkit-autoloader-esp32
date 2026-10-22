#pragma once

#include <Arduino.h>
#include <LittleFS.h>

extern char portalManifestVersion[17];

String portalMimeType(const String &path);
String portalNormalizePath(const char *uri);
bool portalRewriteMirrorPath(String &path, const char *logTag);
bool portalIsRegularFile(File &file);
bool portalIsManifest(const String &mime);
String portalBuildManifest(File &file);
void portalSeedManifestVersion();
bool portalMountStorage();
void portalLogHeap(const char *context);
