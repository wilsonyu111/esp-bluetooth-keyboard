#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>
#include "device_config.h"
#include "keymap.h"
#include "combo.h"
#include "messageQueue.h"
#include "processQueue.h"
#include "webserver.h"

HijelHID_BLEKeyboard* keyboard = nullptr;

void setup() {
    Serial.begin(115200);
    delay(200);

    // NVS is ready after Arduino init; load names before constructing BLE.
    deviceConfigBegin();

    keyboard = new HijelHID_BLEKeyboard(deviceConfigBleName());
    keyboard->setLogLevel(HIDLogLevel::Normal);
    keyboard->begin();
    Serial.printf("BLE keyboard advertising as: %s\n", deviceConfigBleName());

    keymapBegin();
    comboBegin();
    queueInit();
    startWifiAndServer();
}

void loop() {
    if (!handleQueue()) {
        delay(1);
    }
}
