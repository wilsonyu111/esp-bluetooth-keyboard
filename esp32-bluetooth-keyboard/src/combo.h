#ifndef COMBO_H
#define COMBO_H

#include "messageQueue.h"

static const uint8_t COMBO_NAME_MAX = 20;
static const uint8_t COMBO_KEY_MAX = 30;
static const uint8_t COMBO_HOLD_MAX = 6;
static const uint8_t COMBO_SLOT_COUNT = 20;

void comboBegin();
uint8_t comboStoredCount();
String comboListJson();

// keys is a comma-separated list of catalog ids.
// holdTime and waitAfterTap are one millisecond value each.
// When hold is true, a keyboard combo may include modifiers plus at most COMBO_HOLD_MAX other keys.
// On failure, error points at a stable literal.
bool comboAdd(const char* name, bool hold, const char* keys, const char* holdTime, const char* waitAfterTap, const char** error);
bool comboDelete(uint8_t index);
bool comboFire(uint8_t index);

#endif
