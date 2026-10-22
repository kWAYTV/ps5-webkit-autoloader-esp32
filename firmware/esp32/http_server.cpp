#include <WebServer.h>
#include <LittleFS.h>
#include "http_server.h"
#include "host_files.h"
#include "led.h"
#include "config.h"

static WebServer webServer(80);

static void setHttpNoCacheHeaders()
{
    webServer.sendHeader("Cache-Control", PORTAL_NO_CACHE);
    webServer.sendHeader("Pragma", "no-cache");
    webServer.sendHeader("Expires", "0");
}

static void logHttpRequest()
{
    portalLedActivity();

    const char *method = webServer.method() == HTTP_GET ? "GET" :
                         webServer.method() == HTTP_POST ? "POST" : "OTHER";

    Serial.printf("[HTTP] %s %s", method, webServer.uri().c_str());

    for (int i = 0; i < webServer.args(); i++)
    {
        Serial.printf(
            "%c%s=%s",
            i == 0 ? '?' : '&',
            webServer.argName(i).c_str(),
            webServer.arg(i).c_str()
        );
    }

    Serial.println();
}

static void httpNetworkTestHandler()
{
    portalLogHeap("HTTP network test");
    logHttpRequest();
    Serial.printf("[HTTP] content_len=%d\n", webServer.client().available());
    setHttpNoCacheHeaders();
    webServer.send(200, "text/plain", "OK");
}

static void httpFileHandler()
{
    portalLogHeap("HTTP file begin");
    logHttpRequest();

    String path = portalNormalizePath(webServer.uri().c_str());

    if (path.startsWith("/networktest/"))
    {
        httpNetworkTestHandler();
        return;
    }

    portalRewriteMirrorPath(path, "HTTP");

    if (path.endsWith("/"))
    {
        path += "index.html";
    }
    else if (!LittleFS.exists(path))
    {
        String indexPath = path + "/index.html";
        if (LittleFS.exists(indexPath))
            path = indexPath;
    }

    bool gzip = false;
    String gzPath = path + ".gz";
    File file;
    if (!portalIsManifest(portalMimeType(path)))
        file = LittleFS.open(gzPath, "r");

    if (file && !file.isDirectory()) {
        gzip = true;
    } else {
        file = LittleFS.open(path, "r");
    }

    if (!file || file.isDirectory())
    {
        if (file)
            file.close();

        Serial.printf("[HTTP] 404: %s", path.c_str());

        if (path.startsWith("/document/") && (path.indexOf("/ps5") >= 0 || path.indexOf("/ps4") >= 0))
        {
            Serial.printf(" | Redirect: %s -> %s\n", path.c_str(), PORTAL_REDIRECT_URL);
            webServer.sendHeader("Location", PORTAL_REDIRECT_URL, true);
            webServer.send(302, "text/plain", "");
        }
        else
        {
            Serial.print("\n");
            webServer.send(404, "text/plain", "404 Not Found");
        }
        return;
    }

    String contentType = portalMimeType(path);

    if (portalIsManifest(contentType))
    {
        String manifest = portalBuildManifest(file);
        file.close();
        setHttpNoCacheHeaders();
        webServer.send(200, "text/cache-manifest; charset=utf-8", manifest);
        portalLogHeap("HTTP manifest end");
        return;
    }

    if (gzip && contentType == "application/octet-stream")
        webServer.sendHeader("Content-Encoding", "gzip");

    webServer.streamFile(file, contentType);
    file.close();
    portalLogHeap("HTTP file end");
}

void portalStartHttp()
{
    webServer.on("/generate_204", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(204);
    });

    webServer.on("/gen_204", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(204);
    });

    webServer.on("/hotspot-detect.html", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(200, "text/html", "Success");
    });

    webServer.on("/connecttest.txt", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(200, "text/plain", "Microsoft Connect Test");
    });

    webServer.on("/ncsi.txt", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(200, "text/plain", "Microsoft NCSI");
    });

    webServer.on("/redirect", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(200, "text/plain", "Success");
    });

    webServer.on("/netstart/icst", HTTP_GET, [](){
        logHttpRequest();
        setHttpNoCacheHeaders();
        webServer.send(200, "text/plain", "Success");
    });

    webServer.onNotFound(httpFileHandler);
    webServer.begin();
    Serial.println("HTTP: OK");
}

void portalProcessHttp()
{
    webServer.handleClient();
}
