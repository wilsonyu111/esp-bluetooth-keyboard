#include "webserver.h"
#include "keymap.h"
#include "combo.h"
#include "helldiver.h"
#include "messageQueue.h"
#include "wifi_config.h"
#include "device_config.h"
#include <HijelHID_BLEKeyboard.h>

AsyncWebServer* server = nullptr;
uint16_t serverPort = 80;
bool routesRegistered = false;

extern HijelHID_BLEKeyboard* keyboard;

static bool initLittleFS() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    Serial.println("LittleFS Mounted Successfully");

    File root = LittleFS.open("/");
    if (root && root.isDirectory()) {
        Serial.println("Files in LittleFS:");
        File file = root.openNextFile();
        while (file) {
            Serial.print("  ");
            Serial.print(file.name());
            Serial.print(" (");
            Serial.print(file.size());
            Serial.println(" bytes)");
            file = root.openNextFile();
        }
    }
    root.close();
    return true;
}

static const AsyncWebParameter* getRequestParam(AsyncWebServerRequest* request, const char* name) {
    if (request->hasParam(name, true)) {
        return request->getParam(name, true);
    }
    if (request->hasParam(name)) {
        return request->getParam(name);
    }
    return nullptr;
}

static bool saveSlotsFromRequest(AsyncWebServerRequest* request) {
    for (uint8_t i = 0; i < KEYMAP_SLOT_COUNT; ++i) {
        char idKey[4];
        char modKey[4];
        snprintf(idKey, sizeof(idKey), "s%u", i);
        snprintf(modKey, sizeof(modKey), "m%u", i);

        const AsyncWebParameter* idParam = getRequestParam(request, idKey);
        if (idParam == nullptr) {
            return false;
        }

        uint8_t modifiers = 0;
        const AsyncWebParameter* modParam = getRequestParam(request, modKey);
        if (modParam != nullptr) {
            modifiers = (uint8_t)modParam->value().toInt();
        }

        if (!keymapSetSlot(i, idParam->value().c_str(), modifiers)) {
            return false;
        }
    }

    return keymapSave();
}

void initWebServer(uint16_t port) {
    serverPort = port;

    if (!initLittleFS()) {
        Serial.println("Failed to initialize LittleFS");
        return;
    }

    if (server != nullptr) {
        server->end();
        delete server;
        server = nullptr;
        routesRegistered = false;
    }

    server = new AsyncWebServer(serverPort);

    if (!routesRegistered) {
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

        auto serveController = [](AsyncWebServerRequest* request) {
            if (LittleFS.exists("/controller.html")) {
                request->send(LittleFS, "/controller.html", "text/html");
            } else {
                Serial.println("ERROR: controller.html not found in LittleFS");
                request->send(404, "text/plain", "File not found: controller.html. Please upload filesystem.");
            }
        };

        server->on("/", HTTP_GET, serveController);
        server->on("/controller", HTTP_GET, serveController);
        server->on("/controller.html", HTTP_GET, serveController);

        server->on("/config", HTTP_GET, [](AsyncWebServerRequest* request) {
            if (LittleFS.exists("/config.html")) {
                request->send(LittleFS, "/config.html", "text/html");
            } else {
                Serial.println("ERROR: config.html not found in LittleFS");
                request->send(404, "text/plain", "File not found: config.html. Please upload filesystem.");
            }
        });

        server->on("/config.html", HTTP_GET, [](AsyncWebServerRequest* request) {
            if (LittleFS.exists("/config.html")) {
                request->send(LittleFS, "/config.html", "text/html");
            } else {
                request->send(404, "text/plain", "File not found: config.html");
            }
        });

        auto serveHelldiver = [](AsyncWebServerRequest* request) {
            if (LittleFS.exists("/helldiver.html")) {
                request->send(LittleFS, "/helldiver.html", "text/html");
            } else {
                Serial.println("ERROR: helldiver.html not found in LittleFS");
                request->send(404, "text/plain", "File not found: helldiver.html. Please upload filesystem.");
            }
        };

        server->on("/helldiver", HTTP_GET, serveHelldiver);
        server->on("/helldiver.html", HTTP_GET, serveHelldiver);
        // Prefer the real file; gzip-first probes for missing *.png.gz and spams LittleFS open errors.
        server->serveStatic("/hd-icons/", LittleFS, "/hd-icons/").setTryGzipFirst(false);

        server->on(AsyncURIMatcher::all(), HTTP_OPTIONS, [](AsyncWebServerRequest* request) {
            request->send(204);
        });

        server->on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
            if (LittleFS.exists("/style.css")) {
                request->send(LittleFS, "/style.css", "text/css");
            } else {
                Serial.println("ERROR: style.css not found in LittleFS");
                request->send(404, "text/plain", "File not found: style.css");
            }
        });

        server->on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
            bool paired = keyboard != nullptr && keyboard->isPaired();
            String json = String("{\"paired\":") + (paired ? "true" : "false") + "}";
            request->send(200, "application/json", json);
        });

        server->on("/api/catalog", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", keymapCatalogJson());
        });

        server->on("/api/slots", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", keymapSlotsJson());
        });

        server->on("/api/slots", HTTP_POST, [](AsyncWebServerRequest* request) {
            if (!saveSlotsFromRequest(request)) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_slots\"}");
                return;
            }
            request->send(200, "application/json", String("{\"ok\":true,") + keymapSlotsJson().substring(1));
        });

        server->on("/api/helldiver", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", helldiverListJson());
        });

        server->on("/api/helldiver_fire", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* indexParam = getRequestParam(request, "index");
            if (indexParam == nullptr) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_index\"}");
                return;
            }

            int index = indexParam->value().toInt();
            if (index < 0 || index >= (int)helldiverCount()) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_index\"}");
                return;
            }

            uint16_t holdTime = 100;
            uint16_t waitAfterTap = 100;
            const AsyncWebParameter* holdParam = getRequestParam(request, "holdTime");
            const AsyncWebParameter* waitParam = getRequestParam(request, "waitAfterTap");
            if (holdParam != nullptr) {
                long value = holdParam->value().toInt();
                if (value < 0) {
                    value = 0;
                }
                if (value > 65535) {
                    value = 65535;
                }
                holdTime = (uint16_t)value;
            }
            if (waitParam != nullptr) {
                long value = waitParam->value().toInt();
                if (value < 0) {
                    value = 0;
                }
                if (value > 65535) {
                    value = 65535;
                }
                waitAfterTap = (uint16_t)value;
            }

            if (keyboard == nullptr || !keyboard->isPaired()) {
                request->send(503, "application/json", "{\"ok\":false,\"error\":\"not_paired\"}");
                return;
            }

            if (!helldiverFire((uint8_t)index, holdTime, waitAfterTap)) {
                if (!queueHasRoom(1)) {
                    request->send(503, "application/json", "{\"ok\":false,\"error\":\"queue_full\"}");
                } else {
                    request->send(400, "application/json", "{\"ok\":false,\"error\":\"fire_failed\"}");
                }
                return;
            }

            request->send(200, "application/json", "{\"ok\":true}");
        });

        server->on("/api/combos", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", comboListJson());
        });

        server->on("/api/combos", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* nameParam = getRequestParam(request, "name");
            const AsyncWebParameter* keysParam = getRequestParam(request, "keys");
            if (nameParam == nullptr || keysParam == nullptr) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_fields\"}");
                return;
            }

            bool hold = false;
            const AsyncWebParameter* holdParam = getRequestParam(request, "hold");
            if (holdParam != nullptr) {
                hold = holdParam->value() == "1";
            }

            String name = nameParam->value();
            String keys = keysParam->value();
            String holdTime;
            String waitAfterTap;
            const AsyncWebParameter* holdTimeParam = getRequestParam(request, "holdTime");
            const AsyncWebParameter* waitParam = getRequestParam(request, "waitAfterTap");
            if (holdTimeParam != nullptr) {
                holdTime = holdTimeParam->value();
            }
            if (waitParam != nullptr) {
                waitAfterTap = waitParam->value();
            }

            const char* error = nullptr;
            if (!comboAdd(name.c_str(), hold, keys.c_str(), holdTime.c_str(), waitAfterTap.c_str(), &error)) {
                String body = String("{\"ok\":false,\"error\":\"") + (error != nullptr ? error : "save_failed") + "\"}";
                request->send(400, "application/json", body);
                return;
            }
            request->send(200, "application/json", String("{\"ok\":true,") + comboListJson().substring(1));
        });

        server->on("/api/combos_delete", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* indexParam = getRequestParam(request, "index");
            if (indexParam == nullptr) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_index\"}");
                return;
            }

            int index = indexParam->value().toInt();
            if (index < 0 || index >= comboStoredCount()) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_index\"}");
                return;
            }

            if (!comboDelete((uint8_t)index)) {
                request->send(500, "application/json", "{\"ok\":false,\"error\":\"save_failed\"}");
                return;
            }
            request->send(200, "application/json", String("{\"ok\":true,") + comboListJson().substring(1));
        });

        server->on("/api/combos_fire", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* indexParam = getRequestParam(request, "index");
            if (indexParam == nullptr) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_index\"}");
                return;
            }

            int index = indexParam->value().toInt();
            if (index < 0 || index >= comboStoredCount()) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_index\"}");
                return;
            }

            if (keyboard == nullptr || !keyboard->isPaired()) {
                request->send(503, "application/json", "{\"ok\":false,\"error\":\"not_paired\"}");
                return;
            }

            if (!comboFire((uint8_t)index)) {
                if (queueFree() < COMBO_KEY_MAX) {
                    request->send(503, "application/json", "{\"ok\":false,\"error\":\"queue_full\"}");
                } else {
                    request->send(400, "application/json", "{\"ok\":false,\"error\":\"fire_failed\"}");
                }
                return;
            }

            request->send(200, "application/json", "{\"ok\":true}");
        });

        server->on("/api/type", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* textParam = getRequestParam(request, "text");
            if (textParam == nullptr || textParam->value().length() == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_text\"}");
                return;
            }

            String text = textParam->value();
            if (text.length() > QUEUE_TEXT_MAX) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"too_long\"}");
                return;
            }

            if (keyboard == nullptr || !keyboard->isPaired()) {
                request->send(503, "application/json", "{\"ok\":false,\"error\":\"not_paired\"}");
                return;
            }

            if (!queuePushText(text.c_str(), text.length())) {
                request->send(503, "application/json", "{\"ok\":false,\"error\":\"queue_full\"}");
                return;
            }

            Serial.printf("text queued (%u chars)\n", (unsigned)text.length());
            request->send(200, "application/json", "{\"ok\":true}");
        });

        server->on("/api/reset_slot", HTTP_POST, [](AsyncWebServerRequest* request) {
            if (!request->hasParam("slot")) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_slot\"}");
                return;
            }

            int slot = request->getParam("slot")->value().toInt();
            if (slot < 0 || slot >= KEYMAP_SLOT_COUNT) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_slot\"}");
                return;
            }

            if (!keymapResetSlot((uint8_t)slot)) {
                request->send(500, "application/json", "{\"ok\":false,\"error\":\"reset_failed\"}");
                return;
            }
            request->send(200, "application/json", String("{\"ok\":true,") + keymapSlotsJson().substring(1));
        });

        server->on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", wifiConfigJson());
        });

        server->on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* ssidParam = getRequestParam(request, "ssid");
            if (ssidParam == nullptr || ssidParam->value().length() == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_ssid\"}");
                return;
            }

            String ssid = ssidParam->value();
            String password;
            const AsyncWebParameter* passwordParam = getRequestParam(request, "password");
            if (passwordParam != nullptr) {
                password = passwordParam->value();
            }

            if (ssid.length() > WIFI_SSID_MAX || password.length() > WIFI_PASSWORD_MAX) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"too_long\"}");
                return;
            }

            if (!wifiConfigSave(ssid.c_str(), password.c_str())) {
                request->send(500, "application/json", "{\"ok\":false,\"error\":\"save_failed\"}");
                return;
            }

            request->send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
            Serial.println("WiFi: credentials saved, rebooting...");
            delay(500);
            ESP.restart();
        });

        server->on("/api/device", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", deviceConfigJson());
        });

        server->on("/api/device", HTTP_POST, [](AsyncWebServerRequest* request) {
            const AsyncWebParameter* softApParam = getRequestParam(request, "softApSsid");
            const AsyncWebParameter* bleParam = getRequestParam(request, "bleName");
            if (softApParam == nullptr || softApParam->value().length() == 0 ||
                bleParam == nullptr || bleParam->value().length() == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_fields\"}");
                return;
            }

            String softApSsid = softApParam->value();
            String bleName = bleParam->value();
            if (softApSsid.length() > DEVICE_SOFTAP_SSID_MAX || bleName.length() > DEVICE_BLE_NAME_MAX) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"too_long\"}");
                return;
            }

            if (!deviceConfigSave(softApSsid.c_str(), bleName.c_str())) {
                request->send(500, "application/json", "{\"ok\":false,\"error\":\"save_failed\"}");
                return;
            }

            request->send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
            Serial.println("Device: names saved, rebooting...");
            delay(500);
            ESP.restart();
        });

        server->on("/key", HTTP_GET, [](AsyncWebServerRequest* request) {
            if (!request->hasParam("slot")) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_slot\"}");
                return;
            }

            int slot = request->getParam("slot")->value().toInt();
            if (slot < 0 || slot >= KEYMAP_SLOT_COUNT) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_slot\"}");
                return;
            }

            if (keyboard == nullptr || !keyboard->isPaired()) {
                request->send(503, "application/json", "{\"ok\":false,\"error\":\"not_paired\"}");
                return;
            }

            if (!keymapFireSlot((uint8_t)slot)) {
                request->send(503, "application/json", "{\"ok\":false,\"error\":\"queue_full\"}");
                return;
            }

            Serial.printf("key slot %d queued\n", slot);
            request->send(200, "application/json", "{\"ok\":true}");
        });

        server->onNotFound([](AsyncWebServerRequest* request) {
            Serial.print("404 - Not found: ");
            Serial.println(request->url());
            request->send(404, "text/plain", "Not found");
        });

        routesRegistered = true;
        Serial.println("Web server routes registered");
    }
}

void startWebServer() {
    if (server == nullptr) {
        Serial.println("Cannot start web server: server not initialized");
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        server->begin();
        Serial.print("Web server started on port ");
        Serial.println(serverPort);
        Serial.print("Access the portal at: http://");
        Serial.println(WiFi.localIP());
    } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        server->begin();
        Serial.print("Web server started on port ");
        Serial.println(serverPort);
        Serial.print("Access the portal at: http://");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("Cannot start web server: WiFi not initialized");
    }
}

void stopWebServer() {
    if (server != nullptr) {
        server->end();
        Serial.println("Web server stopped");
    }
}

static bool connectWifiSta() {
    if (!wifiConfigConfigured()) {
        Serial.println("WiFi: no SSID in NVS");
        return false;
    }

    const char* ssid = wifiConfigSsid();
    Serial.printf("WiFi: connecting to '%s'...\n", ssid);
    // Modem sleep must stay enabled when BLE is also running (ESP-IDF aborts otherwise).
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_MIN_MODEM);
    WiFi.begin(ssid, wifiConfigPassword());

    const uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startMs >= WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("WiFi: STA connect timed out");
            return false;
        }
        delay(250);
        Serial.print('.');
    }
    Serial.println();
    Serial.print("WiFi: connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

static void startSoftApFallback() {
    const char* softApSsid = deviceConfigSoftApSsid();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(softApSsid);
    IPAddress apIp(192, 168, 4, 1);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));

    Serial.printf("WiFi: SoftAP fallback SSID: %s, IP: ", softApSsid);
    Serial.println(apIp);
}

void startWifiAndServer(bool forceSoftAp) {
    deviceConfigBegin();
    wifiConfigBegin();

    if (forceSoftAp) {
        Serial.println("WiFi: SoftAP forced (double reset)");
        startSoftApFallback();
    } else if (!connectWifiSta()) {
        startSoftApFallback();
    }

    initWebServer(80);
    startWebServer();
}
