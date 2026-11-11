#include "processQueue.h"
#include "messageQueue.h"
#include <HijelHID_BLEKeyboard.h>

extern HijelHID_BLEKeyboard* keyboard;

// Static: QueuedKey includes textByte[251], so a chord buffer is too large for the stack.
static QueuedKey s_playKeys[QUEUE_CHORD_MAX];

static void playText(const QueuedKey& key) {
    if (keyboard == nullptr || key.textByte[0] == '\0') {
        return;
    }
    keyboard->print(key.textByte);
}

static void playTap(const QueuedKey& key) {
    if (keyboard == nullptr) {
        return;
    }
    if (key.kind == KeyKind::Text) {
        playText(key);
        return;
    }
    if (key.kind == KeyKind::Media) {
        keyboard->press(key.code);
        delay(key.timeToHold);
        keyboard->release(key.code);
    } else {
        keyboard->press((uint8_t)key.code);
        delay(key.timeToHold);
        keyboard->release((uint8_t)key.code);
    }
    delay(key.timeToWaitAfterTap);
}

static void playChord(const QueuedKey* keys, uint8_t count) {
    if (count == 0 || keyboard == nullptr) {
        return;
    }
    if (keys[0].kind == KeyKind::Media) {
        playTap(keys[0]);
        return;
    }

    for (uint8_t i = 0; i < count; ++i) {
        keyboard->press((uint8_t)keys[i].code);
    }
    delay(keys[0].timeToHold);
    keyboard->releaseAll();
    delay(keys[0].timeToWaitAfterTap);
}

bool handleQueue(void) {
    uint8_t count = queuePopGroup(s_playKeys, QUEUE_CHORD_MAX);
    if (count == 0) {
        return false;
    }
    if (s_playKeys[0].kind == KeyKind::Text || s_playKeys[0].chord == 0) {
        playTap(s_playKeys[0]);
    } else {
        playChord(s_playKeys, count);
    }
    return true;
}
