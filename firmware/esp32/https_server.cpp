#include <Arduino.h>
#include <LittleFS.h>
#include "esp_https_server.h"
#include "server_certs.h"
#include "https_server.h"
#include "host_files.h"
#include "config.h"
#include "led.h"

#if USB_DEBUG
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdarg.h>
#include <sys/socket.h>
#include <errno.h>
#endif

static httpd_handle_t httpsServer = NULL;

#if USB_DEBUG
static int tlsDebugVprintf(const char *format, va_list args)
{
    char buffer[256];
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    if (length <= 0)
        return length;
    size_t outputLength = length < (int)sizeof(buffer) ? (size_t)length : sizeof(buffer) - 1;
    return (int)Serial.write((const uint8_t *)buffer, outputLength);
}

static void logTlsMemory(const char *stage)
{
    Serial.printf("[TLS] %s: free=%u largest_internal=%u\n", stage,
                  ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

static void httpsSessionCreated(esp_https_server_user_cb_arg_t *event)
{
    Serial.printf("[TLS] handshake OK: fd=%d\n", event->tls->sockfd);
    logTlsMemory("after handshake");
}

static const char *clientHelloSni(const uint8_t *data, size_t size,
                                  char *host, size_t hostSize)
{
    if (size < 5) return "incomplete";
    if (data[0] != 22) return "not ClientHello";
    size_t recordEnd = 5 + (((size_t)data[3] << 8) | data[4]);
    if (recordEnd > size) return "incomplete";
    if (recordEnd < 9 || data[5] != 1) return "not ClientHello";
    size_t helloEnd = 9 + (((size_t)data[6] << 16) | ((size_t)data[7] << 8) | data[8]);
    if (helloEnd > recordEnd) return "incomplete";
    size_t pos = 9 + 2 + 32;
    if (pos >= helloEnd) return "malformed";
    pos += 1 + data[pos];
    if (pos + 2 > helloEnd) return "malformed";
    pos += 2 + (((size_t)data[pos] << 8) | data[pos + 1]);
    if (pos >= helloEnd) return "malformed";
    pos += 1 + data[pos];
    if (pos + 2 > helloEnd) return "no SNI";
    size_t extensionsEnd = pos + 2 + (((size_t)data[pos] << 8) | data[pos + 1]);
    pos += 2;
    if (extensionsEnd > helloEnd) return "malformed";
    while (pos + 4 <= extensionsEnd) {
        uint16_t type = ((uint16_t)data[pos] << 8) | data[pos + 1];
        size_t end = pos + 4 + (((size_t)data[pos + 2] << 8) | data[pos + 3]);
        if (end > extensionsEnd) return "malformed";
        if (type == 0) {
            size_t name = pos + 6;
            if (name + 3 > end || data[name] != 0) return "malformed SNI";
            size_t length = ((size_t)data[name + 1] << 8) | data[name + 2];
            name += 3;
            if (!length || name + length > end || length >= hostSize) return "malformed SNI";
            memcpy(host, data + name, length);
            host[length] = 0;
            return host;
        }
        pos = end;
    }
    return "no SNI";
}

static __attribute__((noinline)) const char *probeClientHelloSni(int sockfd, char *host, size_t hostSize)
{
    uint8_t hello[3072];
    const char *sni = "unavailable";
    for (int attempt = 0; attempt < 10; ++attempt) {
        int size = recv(sockfd, hello, sizeof(hello), MSG_PEEK | MSG_DONTWAIT);
        if (size > 0) {
            sni = clientHelloSni(hello, (size_t)size, host, hostSize);
            if (strcmp(sni, "incomplete") != 0) break;
        } else if (size == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            sni = "socket closed";
            break;
        }
        delay(10);
    }
    return sni;
}

extern "C" int __real_esp_tls_server_session_create(esp_tls_cfg_server_t *, int, esp_tls_t *);
extern "C" int __wrap_esp_tls_server_session_create(esp_tls_cfg_server_t *cfg,
                                                      int sockfd, esp_tls_t *tls)
{
    char host[256];
    const char *sni = probeClientHelloSni(sockfd, host, sizeof(host));
    Serial.printf("[TLS] ClientHello fd=%d sni=%s\n", sockfd, sni);
    logTlsMemory("before handshake");
    int result = __real_esp_tls_server_session_create(cfg, sockfd, tls);
    Serial.printf("[TLS] handshake fd=%d sni=%s result=%d\n", sockfd, sni, result);
    if (result != 0)
        logTlsMemory("handshake failed");
    return result;
}
#endif

void portalPrepareHttpsDebug()
{
#if USB_DEBUG
    esp_log_set_vprintf(tlsDebugVprintf);
#endif
}

static void setHttpsNoCacheHeaders(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", PORTAL_NO_CACHE);
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
}

static esp_err_t httpsFileHandler(httpd_req_t *req)
{
    portalLedActivity();
    portalLogHeap("HTTPS file begin");

    Serial.printf("[HTTPS] method=%d uri=%s\n", req->method, req->uri);

    char buf[256];
    if (httpd_req_get_hdr_value_str(req, "Host", buf, sizeof(buf)) == ESP_OK)
        Serial.printf("[HTTPS] Host: %s\n", buf);
    if (httpd_req_get_hdr_value_str(req, "User-Agent", buf, sizeof(buf)) == ESP_OK)
        Serial.printf("[HTTPS] UA: %s\n", buf);

    String path = portalNormalizePath(req->uri);
    portalRewriteMirrorPath(path, "HTTPS");

    File file;
    if (path.endsWith("/"))
    {
        path += "index.html";
    }
    else
    {
        file = LittleFS.open(path, "r");
        if (!portalIsRegularFile(file))
        {
            if (file)
                file.close();
            String indexPath = path + "/index.html";
            if (LittleFS.exists(indexPath))
                path = indexPath;
        }
    }

    bool gzip = false;
    String gzPath = path + ".gz";
    File gzFile;
    if (!portalIsManifest(portalMimeType(path)))
        gzFile = LittleFS.open(gzPath, "r");

    if (portalIsRegularFile(gzFile)) {
        if (file)
            file.close();
        file = gzFile;
        gzip = true;
    } else {
        if (gzFile)
            gzFile.close();
        if (!portalIsRegularFile(file))
            file = LittleFS.open(path, "r");
    }

    if (!portalIsRegularFile(file))
    {
        if (file)
            file.close();

        Serial.printf("[HTTPS] 404: %s", path.c_str());

        if (path.startsWith("/document/") && path.indexOf("/ps5") >= 0)
        {
            Serial.printf(" | Redirect: %s -> %s\n", path.c_str(), PORTAL_REDIRECT_URL);
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", PORTAL_REDIRECT_URL);
            httpd_resp_set_hdr(req, "Connection", "close");
            httpd_resp_send(req, nullptr, 0);
            return ESP_FAIL;
        }

        Serial.print("\n");
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, "404 Not Found", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    if (portalIsManifest(portalMimeType(path)))
    {
        String manifest = portalBuildManifest(file);
        file.close();
        setHttpsNoCacheHeaders(req);
        httpd_resp_set_type(req, "text/cache-manifest; charset=utf-8");
        esp_err_t result = httpd_resp_send(req, manifest.c_str(), manifest.length());
        portalLogHeap("HTTPS manifest end");
        return result;
    }

    if (gzip)
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    httpd_resp_set_type(req, portalMimeType(path).c_str());

    char buffer[1024];
    while (file.available())
    {
        size_t len = file.readBytes(buffer, sizeof(buffer));
        if (httpd_resp_send_chunk(req, buffer, len) != ESP_OK)
        {
            file.close();
            return ESP_FAIL;
        }
    }

    file.close();
    httpd_resp_send_chunk(req, NULL, 0);
    portalLogHeap("HTTPS file end");
    return ESP_OK;
}

static esp_err_t httpsFixedUrlHandler(httpd_req_t *req)
{
    portalLedActivity();
    portalLogHeap("HTTPS fixed URL");
    setHttpsNoCacheHeaders(req);

    String uri = req->uri;

    if (uri == "/generate_204" || uri == "/gen_204")
    {
        httpd_resp_set_status(req, "204 No Content");
        return httpd_resp_send(req, nullptr, 0);
    }

    if (uri == "/hotspot-detect.html")
        httpd_resp_set_type(req, "text/html");
    else
        httpd_resp_set_type(req, "text/plain");

    if (uri == "/connecttest.txt")
        return httpd_resp_send(req, "Microsoft Connect Test", HTTPD_RESP_USE_STRLEN);

    if (uri == "/ncsi.txt")
        return httpd_resp_send(req, "Microsoft NCSI", HTTPD_RESP_USE_STRLEN);

    return httpd_resp_send(req, "Success", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t httpsNetworkTestHandler(httpd_req_t *req)
{
    portalLedActivity();
    portalLogHeap("HTTPS network test");
    Serial.printf(
        "[HTTPS %s] uri=%s content_len=%d\n",
        req->method == HTTP_GET ? "GET" : "POST",
        req->uri,
        req->content_len
    );

    char buffer[1024];
    int remaining = req->content_len;
    while (remaining > 0)
    {
        int len = httpd_req_recv(req, buffer, min(remaining, (int)sizeof(buffer)));
        if (len <= 0)
        {
            if (len == HTTPD_SOCK_ERR_TIMEOUT)
                continue;
            return ESP_FAIL;
        }
        remaining -= len;
    }

    setHttpsNoCacheHeaders(req);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

bool portalStartHttps()
{
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
#if USB_DEBUG
    config.user_cb = httpsSessionCreated;
    logTlsMemory("before HTTPS start");
#endif

    config.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
    config.port_secure = 443;
    config.cacert_pem = server_crt;
    config.cacert_len = server_crt_len;
    config.prvtkey_pem = server_key;
    config.prvtkey_len = server_key_len;
    config.httpd.uri_match_fn = httpd_uri_match_wildcard;
    config.httpd.max_uri_handlers = 12;

    Serial.printf("cert len: %u, last: %02X\n",
                  (unsigned)server_crt_len, server_crt[server_crt_len - 1]);
    Serial.printf("key len: %u, last: %02X\n",
                  (unsigned)server_key_len, server_key[server_key_len - 1]);
    Serial.println("Starting httpd_ssl_start...");

    esp_err_t err = httpd_ssl_start(&httpsServer, &config);
    if (err != ESP_OK) {
#if USB_DEBUG
        logTlsMemory("HTTPS start failed");
#endif
        Serial.printf("HTTPS start failed: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println("HTTPS socket started");
#if USB_DEBUG
    logTlsMemory("HTTPS listening");
#endif

    httpd_uri_t netstart_uri = {
        .uri = "/netstart/icst", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t generate_uri = {
        .uri = "/generate_204", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t gen_uri = {
        .uri = "/gen_204", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t hotspot_uri = {
        .uri = "/hotspot-detect.html", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t connecttest_uri = {
        .uri = "/connecttest.txt", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t redirect_uri = {
        .uri = "/redirect", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t ncsi_uri = {
        .uri = "/ncsi.txt", .method = HTTP_GET,
        .handler = httpsFixedUrlHandler, .user_ctx = NULL
    };
    httpd_uri_t post_uri = {
        .uri = "/networktest/*", .method = HTTP_POST,
        .handler = httpsNetworkTestHandler, .user_ctx = NULL
    };
    httpd_uri_t networktest_get_uri = {
        .uri = "/networktest/*", .method = HTTP_GET,
        .handler = httpsNetworkTestHandler, .user_ctx = NULL
    };
    httpd_uri_t file_uri = {
        .uri = "/*", .method = HTTP_GET,
        .handler = httpsFileHandler, .user_ctx = NULL
    };

    const httpd_uri_t *handlers[] = {
        &netstart_uri, &generate_uri, &gen_uri, &hotspot_uri,
        &connecttest_uri, &redirect_uri, &ncsi_uri, &post_uri,
        &networktest_get_uri, &file_uri
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++)
    {
        err = httpd_register_uri_handler(httpsServer, handlers[i]);
        if (err != ESP_OK)
        {
            Serial.printf(
                "HTTPS route registration failed: method=%d uri=%s error=%s\n",
                handlers[i]->method, handlers[i]->uri, esp_err_to_name(err)
            );
            httpd_ssl_stop(httpsServer);
            httpsServer = NULL;
            return false;
        }
    }

    Serial.println("HTTPS: OK");
    return true;
}
