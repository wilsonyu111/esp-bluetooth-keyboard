#include "device_config.h"
#include <Preferences.h>

static Preferences prefs;
static char storedSoftApSsid[DEVICE_SOFTAP_SSID_MAX + 1] = "";
static char storedBleName[DEVICE_BLE_NAME_MAX + 1] = "";
static char storedCurrentIp[DEVICE_CURRENT_IP_MAX + 1] = "";
static bool loaded = false;

static void copyBounded(char* dest, size_t destSize, const char* src) {
    if (destSize == 0) {
        return;
    }
    if (src == nullptr) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}

static void applyDefaults() {
    copyBounded(storedSoftApSsid, sizeof(storedSoftApSsid), DEVICE_SOFTAP_SSID_DEFAULT);
    copyBounded(storedBleName, sizeof(storedBleName), DEVICE_BLE_NAME_DEFAULT);
}

static void appendJsonEscaped(String& json, const char* value) {
    for (const char* p = value; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            json += '\\';
        }
        json += *p;
    }
}

void deviceConfigBegin() {
    if (loaded) {
        return;
    }

    applyDefaults();

    // Call only after Arduino/NVS is initialized (e.g. from setup()).
    if (!prefs.begin("device", true)) {
        // Namespace missing on first boot is normal; keep defaults.
        Serial.println("device: no saved names, using defaults");
        loaded = true;
        return;
    }

    String softAp = prefs.getString("ap", "");
    String ble = prefs.getString("ble", "");
    String currentIp = prefs.getString("current_ip", "");
    prefs.end();

    if (softAp.length() > 0) {
        copyBounded(storedSoftApSsid, sizeof(storedSoftApSsid), softAp.c_str());
    }
    if (ble.length() > 0) {
        copyBounded(storedBleName, sizeof(storedBleName), ble.c_str());
    }
    if (currentIp.length() > 0) {
        copyBounded(storedCurrentIp, sizeof(storedCurrentIp), currentIp.c_str());
    }

    loaded = true;
    Serial.printf("device: SoftAP='%s' BLE='%s' current_ip='%s'\n",
                  storedSoftApSsid, storedBleName, storedCurrentIp);
}

bool deviceConfigSave(const char* softApSsid, const char* bleName) {
    if (softApSsid == nullptr || softApSsid[0] == '\0') {
        return false;
    }
    if (bleName == nullptr || bleName[0] == '\0') {
        return false;
    }
    if (strlen(softApSsid) > DEVICE_SOFTAP_SSID_MAX) {
        return false;
    }
    if (strlen(bleName) > DEVICE_BLE_NAME_MAX) {
        return false;
    }

    if (!prefs.begin("device", false)) {
        Serial.println("device: failed to open Preferences for write");
        return false;
    }

    prefs.putString("ap", softApSsid);
    prefs.putString("ble", bleName);
    prefs.end();

    copyBounded(storedSoftApSsid, sizeof(storedSoftApSsid), softApSsid);
    copyBounded(storedBleName, sizeof(storedBleName), bleName);
    loaded = true;
    Serial.printf("device: saved SoftAP='%s' BLE='%s'\n", storedSoftApSsid, storedBleName);
    return true;
}

bool deviceConfigSaveCurrentIp(const char* ip) {
    if (ip == nullptr || ip[0] == '\0') {
        return false;
    }
    if (strlen(ip) > DEVICE_CURRENT_IP_MAX) {
        return false;
    }

    if (!prefs.begin("device", false)) {
        Serial.println("device: failed to open Preferences for write");
        return false;
    }

    prefs.putString("current_ip", ip);
    prefs.end();

    copyBounded(storedCurrentIp, sizeof(storedCurrentIp), ip);
    loaded = true;
    Serial.printf("device: saved current_ip='%s'\n", storedCurrentIp);
    return true;
}

const char* deviceConfigSoftApSsid() {
    if (!loaded) {
        deviceConfigBegin();
    }
    return storedSoftApSsid;
}

const char* deviceConfigBleName() {
    if (!loaded) {
        deviceConfigBegin();
    }
    return storedBleName;
}

const char* deviceConfigCurrentIp() {
    if (!loaded) {
        deviceConfigBegin();
    }
    return storedCurrentIp;
}

String deviceConfigJson() {
    if (!loaded) {
        deviceConfigBegin();
    }

    String json;
    json.reserve(112 + strlen(storedSoftApSsid) + strlen(storedBleName) + strlen(storedCurrentIp));
    json += "{\"softApSsid\":\"";
    appendJsonEscaped(json, storedSoftApSsid);
    json += "\",\"bleName\":\"";
    appendJsonEscaped(json, storedBleName);
    json += "\",\"current_ip\":\"";
    appendJsonEscaped(json, storedCurrentIp);
    json += "\"}";
    return json;
}
