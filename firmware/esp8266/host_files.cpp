#include "host_files.h"
#include "config.h"

void portalLogHeap(const char *where)
{
    Serial.printf("[HEAP] %s: free=%u\n", where, ESP.getFreeHeap());
}

String portalNormalizePath(const String &uri)
{
    String path = uri;
    int q = path.indexOf('?');
    if (q >= 0)
        path = path.substring(0, q);
    if (!path.length())
        path = "/";
    return path;
}

String portalMimeType(const String &path)
{
    if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html; charset=utf-8";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".js") || path.endsWith(".mjs")) return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".gif")) return "image/gif";
    if (path.endsWith(".svg")) return "image/svg+xml";
    if (path.endsWith(".ico")) return "image/x-icon";
    if (path.endsWith(".appcache") || path.endsWith(".manifest") || path.endsWith(".cache"))
        return "text/cache-manifest";
    if (path.endsWith(".woff")) return "font/woff";
    if (path.endsWith(".woff2")) return "font/woff2";
    return "application/octet-stream";
}

bool portalRewriteMirrorPath(String &path)
{
    if (!path.startsWith(PORTAL_PAYLOAD_MIRROR_PREFIX))
        return false;
    String original = path;
    path = String(PORTAL_PAYLOAD_LOCAL_PREFIX) +
           path.substring(strlen(PORTAL_PAYLOAD_MIRROR_PREFIX));
    Serial.printf("[HTTP] Rewrite: %s -> %s\n", original.c_str(), path.c_str());
    return true;
}
