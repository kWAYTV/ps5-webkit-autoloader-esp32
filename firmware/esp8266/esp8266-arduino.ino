#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <LittleFS.h>
#include "server_certs.h"
#include "config.h"
#include "host_files.h"
#include "led.h"
#include "routes.h"

static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GATEWAY(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

static DNSServer dnsServer;
static ESP8266WebServer webServer(80);
static BearSSL::ESP8266WebServerSecure httpsServer(443);

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    portalLedBegin();
    delay(1000);

    Serial.println("\nBOOT ESP8266");
    Serial.printf("Core: %s, SDK: %s\n", ESP.getCoreVersion().c_str(), ESP.getSdkVersion());
    Serial.printf("Reset: %s\n", ESP.getResetReason().c_str());
    Serial.printf("Flash: configured=%u MB, detected=%u MB, CPU=%u MHz\n",
                  ESP.getFlashChipSize() / 1024 / 1024,
                  ESP.getFlashChipRealSize() / 1024 / 1024,
                  ESP.getCpuFreqMHz());

    if (!LittleFS.begin())
        Serial.println("LittleFS: FAILED");
    else {
        FSInfo info;
        LittleFS.info(info);
        Serial.printf("LittleFS: OK, total=%u, used=%u bytes\n", info.totalBytes, info.usedBytes);
    }

    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    delay(200);
    WiFi.mode(WIFI_AP);

    Serial.printf("softAPConfig: %s\n",
                  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET) ? "OK" : "FAILED");
    bool ap = WiFi.softAP(PORTAL_AP_SSID, PORTAL_AP_PASSWORD);
    Serial.printf("softAP: %s\n", ap ? "OK" : "FAILED");
    if (!ap) {
        portalLedError();
        while (true) {
            portalLedUpdate();
            delay(10);
        }
    }

    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("SSID: %s\n", WiFi.softAPSSID().c_str());
    Serial.printf("Channel: %d\n", WiFi.channel());
    Serial.printf("MAC: %s\n", WiFi.softAPmacAddress().c_str());
    Serial.printf("DNS: %s\n", dnsServer.start(53, "*", AP_IP) ? "OK" : "FAILED");

    Serial.println("Starting HTTP");
    portalSetupRoutes(webServer);
    portalLogHeap("HTTP ready");

    Serial.println("Starting HTTPS");
    static BearSSL::X509List serverCertificate((const char *)server_crt);
    static BearSSL::PrivateKey serverPrivateKey((const char *)server_key);
    httpsServer.getServer().setECCert(
        &serverCertificate, BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN, &serverPrivateKey);
    portalSetupRoutes(httpsServer);
    portalLogHeap("HTTPS ready");
    Serial.println("READY");
    portalLedReady();
}

void loop()
{
    dnsServer.processNextRequest();
    webServer.handleClient();
    httpsServer.handleClient();
    portalLedUpdate();
    delay(2);
}
