#include <Arduino.h>
#include <LittleFS.h>
#include <HijelHID_BLEKeyboard.h>
#include "device_config.h"
#include "keymap.h"
#include "combo.h"
#include "messageQueue.h"
#include "processQueue.h"
#include "webserver.h"

#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_EEPROM   false
#define ESP_DRD_USE_SPIFFS   false
#include <ESP_DoubleResetDetector.h>

static const int DRD_TIMEOUT_SEC = 10;
static const int DRD_ADDRESS = 0;

HijelHID_BLEKeyboard* keyboard = nullptr;
static DoubleResetDetector* drd = nullptr;

#if defined(RGB_BUILTIN)
static bool s_rainbowActive = false;
static uint8_t s_rainbowHue = 0;
static uint32_t s_rainbowLastMs = 0;

static void hsvToRgb(uint8_t hue, uint8_t brightness, uint8_t* r, uint8_t* g, uint8_t* b) {
    const uint8_t region = hue / 43;
    const uint8_t remainder = (hue - (region * 43)) * 6;
    const uint8_t p = 0;
    const uint8_t q = (brightness * (255 - remainder)) >> 8;
    const uint8_t t = (brightness * remainder) >> 8;

    switch (region) {
        case 0:
            *r = brightness;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = brightness;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = brightness;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = brightness;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = brightness;
            break;
        default:
            *r = brightness;
            *g = p;
            *b = q;
            break;
    }
}

static void serviceStatusLed() {
    if (!s_rainbowActive) {
        return;
    }

    const uint32_t now = millis();
    if (now - s_rainbowLastMs < 20) {
        return;
    }
    s_rainbowLastMs = now;

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    hsvToRgb(s_rainbowHue, RGB_BRIGHTNESS, &r, &g, &b);
    neopixelWrite(RGB_BUILTIN, r, g, b);
    s_rainbowHue++;
}
#endif

void setup() {
    Serial.begin(115200);
    delay(200);

#if defined(RGB_BUILTIN)
    // Clear any leftover color from a previous run / SoftAP session.
    s_rainbowActive = false;
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
#endif

    // NVS is ready after Arduino init; load names before constructing BLE.
    deviceConfigBegin();

    // LittleFS must be mounted before DoubleResetDetector (stores /drd.dat).
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed (DRD)");
    }

    bool forceSoftAp = false;
    drd = new DoubleResetDetector(DRD_TIMEOUT_SEC, DRD_ADDRESS);
    if (drd->detectDoubleReset()) {
        Serial.println("Double reset detected");
        forceSoftAp = true;
#if defined(RGB_BUILTIN)
        s_rainbowActive = true;
        s_rainbowHue = 0;
        s_rainbowLastMs = 0;
#endif
    } else {
        Serial.println("No double reset detected");
    }

    keyboard = new HijelHID_BLEKeyboard(deviceConfigBleName());
    keyboard->setLogLevel(HIDLogLevel::Normal);
    keyboard->begin();
    Serial.printf("BLE keyboard advertising as: %s\n", deviceConfigBleName());

    keymapBegin();
    comboBegin();
    queueInit();
    startWifiAndServer(forceSoftAp);
}

void loop() {
    if (drd != nullptr) {
        drd->loop();
    }
#if defined(RGB_BUILTIN)
    serviceStatusLed();
#endif
    if (!handleQueue()) {
        delay(1);
    }
}
