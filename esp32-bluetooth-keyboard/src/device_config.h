#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

static const size_t DEVICE_SOFTAP_SSID_MAX = 32;
static const size_t DEVICE_BLE_NAME_MAX = 29;

static const char DEVICE_SOFTAP_SSID_DEFAULT[] = "ESP32-Media-Keys";
static const char DEVICE_BLE_NAME_DEFAULT[] = "ESP32 Media Test";

void deviceConfigBegin();
bool deviceConfigSave(const char* softApSsid, const char* bleName);
const char* deviceConfigSoftApSsid();
const char* deviceConfigBleName();
String deviceConfigJson();

#endif
