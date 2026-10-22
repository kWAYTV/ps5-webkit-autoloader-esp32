#pragma once

#include <LittleFS.h>
#include "config.h"
#include "host_files.h"
#include "led.h"

template <typename Server>
static void portalNoCache(Server &server)
{
    server.sendHeader("Cache-Control", PORTAL_NO_CACHE);
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
}

template <typename Server>
static void portalLogRequest(Server &server)
{
    portalLedActivity();
    Serial.printf("[HTTP] %s %s", server.method() == HTTP_GET ? "GET" :
                  server.method() == HTTP_POST ? "POST" : "OTHER", server.uri().c_str());
    for (int i = 0; i < server.args(); i++)
        Serial.printf("%c%s=%s", i ? '&' : '?', server.argName(i).c_str(), server.arg(i).c_str());
    Serial.println();
}

template <typename Server>
static void portalFileHandler(Server &server)
{
    portalLogHeap("file begin");
    portalLogRequest(server);
    String path = portalNormalizePath(server.uri());

    if (path.startsWith("/networktest/")) {
        portalNoCache(server);
        server.send(200, "text/plain", "OK");
        return;
    }

    portalRewriteMirrorPath(path);
    if (path.endsWith("/"))
        path += "index.html";
    else if (!LittleFS.exists(path) && LittleFS.exists(path + "/index.html"))
        path += "/index.html";

    bool gzip = false;
    File file = LittleFS.open(path + ".gz", "r");
    if (file && !file.isDirectory()) {
        gzip = true;
    } else {
        if (file)
            file.close();
        file = LittleFS.open(path, "r");
    }
    if (!file || file.isDirectory()) {
        if (file)
            file.close();
        if (path.startsWith("/document/") && (path.indexOf("/ps5") >= 0 || path.indexOf("/ps4") >= 0)) {
            server.sendHeader("Location", PORTAL_REDIRECT_URL, true);
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "404 Not Found");
        }
        return;
    }

    String type = portalMimeType(path);
    if (gzip && type == "application/octet-stream")
        server.sendHeader("Content-Encoding", "gzip");
    server.streamFile(file, type);
    file.close();
    portalLogHeap("file end");
}

template <typename Server>
static void portalSetupRoutes(Server &server)
{
    server.on("/generate_204", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(204);
    });
    server.on("/gen_204", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(204);
    });
    server.on("/hotspot-detect.html", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(200, "text/html", "Success");
    });
    server.on("/connecttest.txt", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(200, "text/plain", "Microsoft Connect Test");
    });
    server.on("/ncsi.txt", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(200, "text/plain", "Microsoft NCSI");
    });
    server.on("/redirect", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(200, "text/plain", "Success");
    });
    server.on("/netstart/icst", HTTP_GET, [&server]() {
        portalLogRequest(server); portalNoCache(server); server.send(200, "text/plain", "Success");
    });
    server.onNotFound([&server]() { portalFileHandler(server); });
    server.begin();
}
