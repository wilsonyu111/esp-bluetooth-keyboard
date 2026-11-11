#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

static const size_t WIFI_SSID_MAX = 32;
static const size_t WIFI_PASSWORD_MAX = 64;
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

void wifiConfigBegin();
bool wifiConfigSave(const char* ssid, const char* password);
const char* wifiConfigSsid();
const char* wifiConfigPassword();
bool wifiConfigConfigured();
String wifiConfigJson();

#endif
