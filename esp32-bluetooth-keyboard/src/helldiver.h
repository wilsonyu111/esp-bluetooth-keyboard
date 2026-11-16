#ifndef HELLDIVER_H
#define HELLDIVER_H

#include <Arduino.h>

uint8_t helldiverCount();
String helldiverListJson();
bool helldiverFire(uint8_t index, uint16_t holdTime, uint16_t waitAfterTap);

#endif
