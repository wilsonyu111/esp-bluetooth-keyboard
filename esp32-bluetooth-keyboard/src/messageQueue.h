#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "keymap.h"

static const uint16_t QUEUE_KEY_BUDGET = 500;
static const uint8_t QUEUE_CHORD_MAX = 30;
static const size_t QUEUE_TEXT_MAX = 250;

struct QueuedKey {
    uint16_t code;
    KeyKind kind;
    uint16_t timeToHold;
    uint16_t timeToWaitAfterTap;
    uint8_t chord;
    char textByte[QUEUE_TEXT_MAX + 1];
};

void queueInit(void);
uint16_t queueFree(void);
bool queueHasRoom(uint16_t count);
bool queuePushKeys(const uint16_t* codes, uint8_t count, KeyKind kind, uint16_t timeToHold, uint16_t timeToWaitAfterTap, bool hold);
bool queuePushText(const char* text, size_t len);
uint8_t queuePopGroup(QueuedKey* out, uint8_t maxOut);
bool queueEmpty(void);
bool queueFull(void);
size_t queueSize(void);

#endif
