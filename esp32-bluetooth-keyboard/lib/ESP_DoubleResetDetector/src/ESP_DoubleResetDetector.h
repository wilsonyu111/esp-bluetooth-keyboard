/****************************************************************************************************************************
  ESP_DoubleResetDetector.h - MIT, v1.3.2 (header-only copy for authority project)
*****************************************************************************************************************************/

#pragma once

#ifndef ESP_DoubleResetDetector_H
#define ESP_DoubleResetDetector_H

#ifndef DOUBLERESETDETECTOR_DEBUG
  #define DOUBLERESETDETECTOR_DEBUG       false
#endif

#include <Arduino.h>

#ifndef ESP_DOUBLE_RESET_DETECTOR_VERSION
  #define ESP_DOUBLE_RESET_DETECTOR_VERSION             "ESP_DoubleResetDetector v1.3.2"
#endif

#if ESP_DRD_USE_EEPROM
  #include <EEPROM.h>
  #ifndef EEPROM_SIZE
    #define EEPROM_SIZE     512
  #endif
  #ifndef EEPROM_START
    #define EEPROM_START    256
  #endif
#elif ( ESP_DRD_USE_LITTLEFS || ESP_DRD_USE_SPIFFS )
  #include <FS.h>
  #ifdef ESP32
    #if ESP_DRD_USE_LITTLEFS
      #include <LittleFS.h>
      #define FileFS        LittleFS
    #else
      #include "SPIFFS.h"
      #define FileFS   SPIFFS
    #endif
  #else
    #include <LittleFS.h>
    #if ESP_DRD_USE_LITTLEFS
      #define FileFS    LittleFS
    #else
      #define FileFS   SPIFFS
    #endif
  #endif
  #define  DRD_FILENAME     "/drd.dat"
#endif

#define DOUBLERESETDETECTOR_FLAG_SET    0xD0D01234
#define DOUBLERESETDETECTOR_FLAG_CLEAR  0xD0D04321

class DoubleResetDetector
{
  public:
    DoubleResetDetector(int timeout, int address)
    {
#if ESP_DRD_USE_EEPROM
      EEPROM.begin(EEPROM_SIZE);
#elif ( ESP_DRD_USE_LITTLEFS || ESP_DRD_USE_SPIFFS )
      if (!FileFS.begin()) {
      }
#endif
      this->timeout = timeout * 1000;
      this->address = address;
      doubleResetDetected = false;
      waitingForDoubleReset = false;
    };

    bool detectDoubleReset()
    {
      doubleResetDetected = detectRecentlyResetFlag();
      if (doubleResetDetected) {
        clearRecentlyResetFlag();
      } else {
        setRecentlyResetFlag();
        waitingForDoubleReset = true;
      }
      return doubleResetDetected;
    };

    bool waitingForDRD() { return waitingForDoubleReset; }

    void loop()
    {
      if (waitingForDoubleReset && millis() > timeout) {
        stop();
      }
    };

    void stop()
    {
      clearRecentlyResetFlag();
      waitingForDoubleReset = false;
    };

    bool doubleResetDetected;

  private:
    uint32_t DOUBLERESETDETECTOR_FLAG;
    unsigned long timeout;
    int address;
    bool waitingForDoubleReset;
    uint32_t doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG_CLEAR;

    bool detectRecentlyResetFlag()
    {
#if (ESP_DRD_USE_EEPROM)
      EEPROM.get(EEPROM_START, DOUBLERESETDETECTOR_FLAG);
      doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG;
#elif ( ESP_DRD_USE_LITTLEFS || ESP_DRD_USE_SPIFFS )
      if (FileFS.exists(DRD_FILENAME)) {
        File file = FileFS.open(DRD_FILENAME, "r");
        if (file) {
          file.readBytes((char *) &DOUBLERESETDETECTOR_FLAG, sizeof(DOUBLERESETDETECTOR_FLAG));
          doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG;
          file.close();
        }
      }
#endif
      doubleResetDetected = (doubleResetDetectorFlag == DOUBLERESETDETECTOR_FLAG_SET);
      return doubleResetDetected;
    };

    void setRecentlyResetFlag()
    {
      doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG_SET;
      DOUBLERESETDETECTOR_FLAG = DOUBLERESETDETECTOR_FLAG_SET;
#if (ESP_DRD_USE_EEPROM)
      EEPROM.put(EEPROM_START, DOUBLERESETDETECTOR_FLAG);
      EEPROM.commit();
#elif ( ESP_DRD_USE_LITTLEFS || ESP_DRD_USE_SPIFFS )
      File file = FileFS.open(DRD_FILENAME, "w");
      if (file) {
        file.write((uint8_t *) &DOUBLERESETDETECTOR_FLAG, sizeof(DOUBLERESETDETECTOR_FLAG));
        file.close();
      }
#endif
    };

    void clearRecentlyResetFlag()
    {
      doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG_CLEAR;
      DOUBLERESETDETECTOR_FLAG = DOUBLERESETDETECTOR_FLAG_CLEAR;
#if (ESP_DRD_USE_EEPROM)
      EEPROM.put(EEPROM_START, DOUBLERESETDETECTOR_FLAG);
      EEPROM.commit();
#elif ( ESP_DRD_USE_LITTLEFS || ESP_DRD_USE_SPIFFS )
      File file = FileFS.open(DRD_FILENAME, "w");
      if (file) {
        file.write((uint8_t *) &DOUBLERESETDETECTOR_FLAG, sizeof(DOUBLERESETDETECTOR_FLAG));
        file.close();
      }
#endif
    };
};
#endif
