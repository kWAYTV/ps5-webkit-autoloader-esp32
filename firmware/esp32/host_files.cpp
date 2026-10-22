#include "host_files.h"
#include "config.h"
#include "esp_random.h"

char portalManifestVersion[17];

void portalLogHeap(const char *context)
{
    Serial.printf(
        "[HEAP] %s: free=%u, min=%u\n",
        context,
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap()
    );
}

String portalMimeType(const String &path)
{
    if (path.endsWith(".html") || path.endsWith(".htm"))
        return "text/html; charset=utf-8";
    if (path.endsWith(".css"))
        return "text/css";
    if (path.endsWith(".js") || path.endsWith(".mjs"))
        return "application/javascript";
    if (path.endsWith(".json"))
        return "application/json";
    if (path.endsWith(".png"))
        return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
        return "image/jpeg";
    if (path.endsWith(".gif"))
        return "image/gif";
    if (path.endsWith(".svg"))
        return "image/svg+xml";
    if (path.endsWith(".ico"))
        return "image/x-icon";
    if (path.endsWith(".appcache") || path.endsWith(".manifest") || path.endsWith(".cache"))
        return "text/cache-manifest";
    if (path.endsWith(".woff"))
        return "font/woff";
    if (path.endsWith(".woff2"))
        return "font/woff2";

    return "application/octet-stream";
}

String portalNormalizePath(const char *uri)
{
    String path = uri;

    int q = path.indexOf('?');
    if (q >= 0)
        path = path.substring(0, q);

    if (path.length() == 0)
        path = "/";

    return path;
}

bool portalRewriteMirrorPath(String &path, const char *logTag)
{
    if (!path.startsWith(PORTAL_PAYLOAD_MIRROR_PREFIX))
        return false;

    String originalPath = path;
    path = String(PORTAL_PAYLOAD_LOCAL_PREFIX) +
           path.substring(strlen(PORTAL_PAYLOAD_MIRROR_PREFIX));

    Serial.printf(
        "[%s] Rewrite: %s -> %s\n",
        logTag,
        originalPath.c_str(),
        path.c_str()
    );
    return true;
}

bool portalIsRegularFile(File &file)
{
    return file && !file.isDirectory();
}

bool portalIsManifest(const String &mime)
{
    return mime == "text/cache-manifest";
}

String portalBuildManifest(File &file)
{
    String manifest = file.readStringUntil('\n');
    manifest += '\n';

    manifest += "# VERSION ";
    manifest += portalManifestVersion;
    manifest += '\n';

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        String comment = line;
        comment.trim();
        if (comment == "# VERSION" || comment.startsWith("# VERSION ") ||
            comment.startsWith("# VERSION\t"))
            continue;
        manifest += line;
        manifest += '\n';
    }

    Serial.printf("[MANIFEST] %s version=%s\n", file.name(), portalManifestVersion);
    return manifest;
}

void portalSeedManifestVersion()
{
    snprintf(portalManifestVersion, sizeof(portalManifestVersion), "%08lx%08lx",
             (unsigned long)esp_random(), (unsigned long)esp_random());
    Serial.printf("[MANIFEST] boot version=%s\n", portalManifestVersion);
}

bool portalMountStorage()
{
    if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
        Serial.println("LittleFS mount failed");
        return false;
    }
    Serial.println("LittleFS mounted");
    return true;
}
