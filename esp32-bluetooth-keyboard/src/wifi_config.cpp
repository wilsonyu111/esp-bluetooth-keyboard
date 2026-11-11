#include "wifi_config.h"
#include <Preferences.h>

static Preferences prefs;
static char storedSsid[WIFI_SSID_MAX + 1] = "";
static char storedPassword[WIFI_PASSWORD_MAX + 1] = "";

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

void wifiConfigBegin() {
    storedSsid[0] = '\0';
    storedPassword[0] = '\0';

    if (!prefs.begin("wifi", true)) {
        Serial.println("wifi: no saved credentials");
        return;
    }

    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("pass", "");
    prefs.end();

    copyBounded(storedSsid, sizeof(storedSsid), ssid.c_str());
    copyBounded(storedPassword, sizeof(storedPassword), password.c_str());

    if (storedSsid[0] == '\0') {
        Serial.println("wifi: NVS empty");
    } else {
        Serial.printf("wifi: loaded SSID '%s' from NVS\n", storedSsid);
    }
}

bool wifiConfigSave(const char* ssid, const char* password) {
    if (ssid == nullptr || ssid[0] == '\0') {
        return false;
    }
    if (strlen(ssid) > WIFI_SSID_MAX) {
        return false;
    }
    if (password == nullptr) {
        password = "";
    }
    if (strlen(password) > WIFI_PASSWORD_MAX) {
        return false;
    }

    if (!prefs.begin("wifi", false)) {
        Serial.println("wifi: failed to open Preferences for write");
        return false;
    }

    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.end();

    copyBounded(storedSsid, sizeof(storedSsid), ssid);
    copyBounded(storedPassword, sizeof(storedPassword), password);
    Serial.printf("wifi: saved SSID '%s' to NVS\n", storedSsid);
    return true;
}

const char* wifiConfigSsid() {
    return storedSsid;
}

const char* wifiConfigPassword() {
    return storedPassword;
}

bool wifiConfigConfigured() {
    return storedSsid[0] != '\0';
}

String wifiConfigJson() {
    String json;
    json.reserve(64 + strlen(storedSsid));
    json += "{\"ssid\":\"";
    for (const char* p = storedSsid; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            json += '\\';
        }
        json += *p;
    }
    json += "\",\"configured\":";
    json += wifiConfigConfigured() ? "true" : "false";
    json += '}';
    return json;
}
