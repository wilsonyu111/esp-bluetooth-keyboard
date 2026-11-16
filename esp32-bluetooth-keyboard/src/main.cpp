#include <Arduino.h>
#include <LittleFS.h>
#include <HijelHID_BLEKeyboard.h>
#include "device_config.h"
#include "keymap.h"
#include "combo.h"
#include "messageQueue.h"
#include "processQueue.h"
#include "status_led.h"
#include "webserver.h"

#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_EEPROM   false
#define ESP_DRD_USE_SPIFFS   false
#include <ESP_DoubleResetDetector.h>

static const int DRD_TIMEOUT_SEC = 10;
static const int DRD_ADDRESS = 0;

HijelHID_BLEKeyboard* keyboard = nullptr;
static DoubleResetDetector* drd = nullptr;

void setup() {
    Serial.begin(115200);
    delay(200);

    // Clear any leftover color from a previous SoftAP session.
    statusLedBegin();

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
    statusLedService();
    if (!handleQueue()) {
        delay(1);
    }
}
