#include "messageQueue.h"
#include <string.h>

#if defined(BOARD_HAS_PSRAM)
#include <esp_heap_caps.h>
#endif

static QueuedKey* s_queue = nullptr;
static uint16_t s_head = 0;
static uint16_t s_tail = 0;
static uint16_t s_count = 0;
static uint8_t s_nextChord = 1;

static bool queueEnsureStorage(void) {
    if (s_queue != nullptr) {
        return true;
    }

#if defined(BOARD_HAS_PSRAM)
    s_queue = (QueuedKey*)heap_caps_calloc(
        QUEUE_KEY_BUDGET,
        sizeof(QueuedKey),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
#endif
    if (s_queue == nullptr) {
        s_queue = (QueuedKey*)calloc(QUEUE_KEY_BUDGET, sizeof(QueuedKey));
    }
    if (s_queue == nullptr) {
        Serial.println("queue: failed to allocate storage");
        return false;
    }
    Serial.printf(
        "queue: allocated %u x %u bytes\n",
        (unsigned)QUEUE_KEY_BUDGET,
        (unsigned)sizeof(QueuedKey)
    );
    return true;
}

void queueInit(void) {
    queueEnsureStorage();
    s_head = 0;
    s_tail = 0;
    s_count = 0;
    s_nextChord = 1;
}

uint16_t queueFree(void) {
    return QUEUE_KEY_BUDGET - s_count;
}

bool queueHasRoom(uint16_t count) {
    return s_queue != nullptr && count > 0 && count <= queueFree();
}

bool queuePushKeys(const uint16_t* codes, uint8_t count, KeyKind kind, uint16_t timeToHold, uint16_t timeToWaitAfterTap, bool hold) {
    if (codes == nullptr || count == 0 || kind == KeyKind::Text || !queueHasRoom(count)) {
        return false;
    }

    uint8_t chord = 0;
    if (hold) {
        chord = s_nextChord;
        s_nextChord++;
        if (s_nextChord == 0) {
            s_nextChord = 1;
        }
    }

    for (uint8_t i = 0; i < count; ++i) {
        QueuedKey key = {};
        key.code = codes[i];
        key.kind = kind;
        key.timeToHold = timeToHold;
        key.timeToWaitAfterTap = timeToWaitAfterTap;
        key.chord = chord;
        key.textByte[0] = '\0';
        s_queue[s_tail] = key;
        s_tail = (uint16_t)((s_tail + 1) % QUEUE_KEY_BUDGET);
        s_count++;
    }
    return true;
}

bool queuePushText(const char* text, size_t len) {
    if (text == nullptr || len == 0 || len > QUEUE_TEXT_MAX || !queueHasRoom(1)) {
        return false;
    }

    QueuedKey key = {};
    key.code = 0;
    key.kind = KeyKind::Text;
    key.timeToHold = 0;
    key.timeToWaitAfterTap = 0;
    key.chord = 0;
    memcpy(key.textByte, text, len);
    key.textByte[len] = '\0';

    s_queue[s_tail] = key;
    s_tail = (uint16_t)((s_tail + 1) % QUEUE_KEY_BUDGET);
    s_count++;
    return true;
}

uint8_t queuePopGroup(QueuedKey* out, uint8_t maxOut) {
    if (out == nullptr || maxOut == 0 || s_count == 0 || s_queue == nullptr) {
        return 0;
    }

    out[0] = s_queue[s_head];
    s_head = (uint16_t)((s_head + 1) % QUEUE_KEY_BUDGET);
    s_count--;
    uint8_t count = 1;
    if (out[0].chord == 0 || out[0].kind == KeyKind::Text) {
        return count;
    }

    while (count < maxOut && s_count > 0 && s_queue[s_head].chord == out[0].chord) {
        out[count] = s_queue[s_head];
        s_head = (uint16_t)((s_head + 1) % QUEUE_KEY_BUDGET);
        s_count--;
        count++;
    }
    return count;
}

bool queueEmpty(void) {
    return s_count == 0;
}

bool queueFull(void) {
    return s_count >= QUEUE_KEY_BUDGET;
}

size_t queueSize(void) {
    return s_count;
}
