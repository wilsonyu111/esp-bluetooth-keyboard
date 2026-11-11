#include "combo.h"
#include "keymap.h"
#include <HijelHID_BLEKeyboard.h>
#include <Preferences.h>
#include <string.h>

static const size_t COMBO_BLOB_MAX = 1536;

static Preferences prefs;

struct Combo {
    char name[COMBO_NAME_MAX + 1];
    uint16_t steps[COMBO_KEY_MAX];
    uint8_t count;
    bool hold;
    KeyKind kind;
    uint16_t timeToHold;
    uint16_t timeToWaitAfterTap;
};

static Combo combos[COMBO_SLOT_COUNT];
static uint8_t comboCount = 0;

static String jsonEscape(const char* text) {
    String out;
    if (text == nullptr) {
        return out;
    }
    out.reserve(strlen(text) + 8);
    for (const char* p = text; *p; ++p) {
        char c = *p;
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out += c;
        }
    }
    return out;
}

static void slotKey(char* buf, size_t len, uint8_t index) {
    snprintf(buf, len, "c%u", index);
}

static bool isModifierCode(uint16_t code) {
    switch (code) {
        case KEY_LCTRL:
        case KEY_LSHIFT:
        case KEY_LALT:
        case KEY_LGUI:
        case KEY_RCTRL:
        case KEY_RSHIFT:
        case KEY_RALT:
        case KEY_RGUI:
            return true;
        default:
            return false;
    }
}

static void printCombo(const char* label, uint8_t index, const Combo& combo) {
    Serial.printf(
        "combos: %s %u name='%s' hold=%u kind=%s count=%u holdTime=%u waitAfterTap=%u\n",
        label,
        index,
        combo.name,
        combo.hold ? 1 : 0,
        combo.kind == KeyKind::Media ? "media" : "keyboard",
        combo.count,
        combo.timeToHold,
        combo.timeToWaitAfterTap
    );
    for (uint8_t k = 0; k < combo.count; ++k) {
        const KeyCatalogEntry* entry = keymapCatalogEntry(combo.steps[k]);
        Serial.printf("  [%u] %s\n", k, entry != nullptr ? entry->id : "?");
    }
}

static uint8_t countTokens(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return 0;
    }
    uint8_t count = 1;
    for (const char* p = text; *p; ++p) {
        if (*p == ',') {
            if (count == 255) {
                return 255;
            }
            count++;
        }
    }
    return count;
}

static bool copyName(const char* text, char* dest, const char** error) {
    if (text == nullptr) {
        *error = "empty_name";
        return false;
    }
    while (*text == ' ') {
        ++text;
    }
    size_t length = strlen(text);
    while (length > 0 && text[length - 1] == ' ') {
        --length;
    }
    if (length == 0) {
        *error = "empty_name";
        return false;
    }
    if (length > COMBO_NAME_MAX) {
        *error = "name_too_long";
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (text[i] == '\n' || text[i] == '\r') {
            *error = "bad_name";
            return false;
        }
    }
    memcpy(dest, text, length);
    dest[length] = '\0';
    return true;
}

static bool nextToken(const char*& cursor, char* buf, size_t bufLen) {
    if (cursor == nullptr || *cursor == '\0') {
        return false;
    }
    size_t used = 0;
    bool overflow = false;
    while (*cursor && *cursor != ',') {
        if (used + 1 < bufLen) {
            buf[used++] = *cursor;
        } else {
            overflow = true;
        }
        ++cursor;
    }
    buf[used] = '\0';
    while (used > 0 && buf[used - 1] == ' ') {
        buf[--used] = '\0';
    }
    size_t start = 0;
    while (buf[start] == ' ') {
        ++start;
    }
    if (start > 0) {
        memmove(buf, buf + start, strlen(buf + start) + 1);
    }
    if (*cursor == ',') {
        ++cursor;
    }
    return !overflow;
}

static bool parseMs(const char* token, uint16_t& out) {
    if (token == nullptr || token[0] == '\0') {
        return false;
    }
    unsigned long value = 0;
    for (const char* p = token; *p; ++p) {
        if (*p < '0' || *p > '9') {
            return false;
        }
        value = value * 10ul + (unsigned long)(*p - '0');
        if (value > 65535ul) {
            return false;
        }
    }
    out = (uint16_t)value;
    return true;
}

static bool readOneTime(const char* text, uint16_t& dest, const char** error) {
    dest = 0;
    if (text == nullptr || text[0] == '\0') {
        return true;
    }
    while (*text == ' ') {
        ++text;
    }
    if (!parseMs(text, dest)) {
        *error = "bad_time";
        return false;
    }
    return true;
}

static bool fillCombo(Combo& combo, const char* name, bool hold, const char* keys, const char* holdTime, const char* waitAfterTap, bool skipUnknown, const char** error) {
    memset(&combo, 0, sizeof(combo));
    if (!copyName(name, combo.name, error)) {
        return false;
    }
    if (!readOneTime(holdTime, combo.timeToHold, error)) {
        return false;
    }
    if (!readOneTime(waitAfterTap, combo.timeToWaitAfterTap, error)) {
        return false;
    }

    const uint8_t keyCount = countTokens(keys);
    if (keyCount == 0) {
        *error = "empty_keys";
        return false;
    }
    if (keyCount > COMBO_KEY_MAX) {
        *error = "too_many_keys";
        return false;
    }

    const char* cursor = keys;
    bool kindSet = false;
    for (uint8_t i = 0; i < keyCount; ++i) {
        char token[48];
        if (!nextToken(cursor, token, sizeof(token)) || token[0] == '\0') {
            if (skipUnknown) {
                continue;
            }
            *error = "unknown_key";
            return false;
        }
        int index = keymapFindById(token);
        if (index < 0) {
            if (skipUnknown) {
                Serial.printf("combos: skipping unknown id '%s'\n", token);
                continue;
            }
            *error = "unknown_key";
            return false;
        }
        const KeyCatalogEntry* entry = keymapCatalogEntry((uint16_t)index);
        if (entry == nullptr) {
            if (skipUnknown) {
                continue;
            }
            *error = "unknown_key";
            return false;
        }
        if (!kindSet) {
            combo.kind = entry->kind;
            kindSet = true;
        } else if (entry->kind != combo.kind) {
            *error = "mixed_kind";
            return false;
        }
        if (combo.count >= COMBO_KEY_MAX) {
            *error = "too_many_keys";
            return false;
        }
        combo.steps[combo.count++] = (uint16_t)index;
    }

    if (combo.count == 0) {
        *error = "empty_keys";
        return false;
    }

    combo.hold = hold;
    if (!hold) {
        return true;
    }

    if (combo.kind == KeyKind::Media) {
        if (combo.count != 1) {
            *error = "media_hold";
            return false;
        }
        return true;
    }

    uint8_t regular = 0;
    for (uint8_t i = 0; i < combo.count; ++i) {
        const KeyCatalogEntry* entry = keymapCatalogEntry(combo.steps[i]);
        if (entry != nullptr && !isModifierCode(entry->code)) {
            regular++;
        }
    }
    if (regular > COMBO_HOLD_MAX) {
        *error = "too_many_keys";
        return false;
    }
    return true;
}

static String joinIds(const Combo& combo) {
    String out;
    for (uint8_t i = 0; i < combo.count; ++i) {
        if (i > 0) {
            out += ',';
        }
        const KeyCatalogEntry* entry = keymapCatalogEntry(combo.steps[i]);
        if (entry != nullptr) {
            out += entry->id;
        }
    }
    return out;
}

static bool encodeCombo(const Combo& combo, String& out) {
    out = "1\n";
    out += combo.name;
    out += '\n';
    out += combo.hold ? '1' : '0';
    out += '\n';
    out += joinIds(combo);
    out += '\n';
    out += combo.timeToHold;
    out += '\n';
    out += combo.timeToWaitAfterTap;
    return out.length() > 0 && out.length() < COMBO_BLOB_MAX;
}

static bool readLine(const char*& cursor, char* buf, size_t bufLen) {
    if (cursor == nullptr || *cursor == '\0' || bufLen == 0) {
        return false;
    }
    size_t used = 0;
    while (*cursor && *cursor != '\n') {
        if (*cursor != '\r' && used + 1 < bufLen) {
            buf[used++] = *cursor;
        }
        ++cursor;
    }
    buf[used] = '\0';
    if (*cursor == '\n') {
        ++cursor;
    }
    return true;
}

static bool decodeCombo(const char* blob, Combo& combo, const char** error) {
    const char* cursor = blob;
    char version[8];
    char name[COMBO_NAME_MAX + 1];
    char holdText[4];
    char keys[COMBO_BLOB_MAX];
    char holdTime[8];
    char waitAfter[8];
    if (!readLine(cursor, version, sizeof(version)) || strcmp(version, "1") != 0) {
        *error = "bad_blob";
        return false;
    }
    if (!readLine(cursor, name, sizeof(name)) || !readLine(cursor, holdText, sizeof(holdText)) || !readLine(cursor, keys, sizeof(keys)) || !readLine(cursor, holdTime, sizeof(holdTime)) || !readLine(cursor, waitAfter, sizeof(waitAfter))) {
        *error = "bad_blob";
        return false;
    }
    bool hold = holdText[0] == '1';
    return fillCombo(combo, name, hold, keys, holdTime, waitAfter, true, error);
}

static bool saveAll() {
    if (!prefs.begin("combos", false)) {
        Serial.println("combos: failed to open Preferences for write");
        return false;
    }

    prefs.putUChar("count", comboCount);
    for (uint8_t i = 0; i < COMBO_SLOT_COUNT; ++i) {
        char key[4];
        slotKey(key, sizeof(key), i);
        if (i >= comboCount) {
            if (prefs.isKey(key)) {
                prefs.remove(key);
            }
            continue;
        }
        String blob;
        if (!encodeCombo(combos[i], blob)) {
            prefs.end();
            return false;
        }
        prefs.putBytes(key, blob.c_str(), blob.length());
    }

    prefs.end();
    Serial.println("combos: saved to NVS");
    return true;
}

static void loadAll() {
    comboCount = 0;
    memset(combos, 0, sizeof(combos));

    if (!prefs.begin("combos", true)) {
        Serial.println("combos: no saved combos");
        return;
    }

    uint8_t stored = prefs.getUChar("count", 0);
    if (stored > COMBO_SLOT_COUNT) {
        stored = COMBO_SLOT_COUNT;
    }

    for (uint8_t i = 0; i < stored; ++i) {
        char key[4];
        slotKey(key, sizeof(key), i);
        char blob[COMBO_BLOB_MAX];
        size_t length = prefs.getBytes(key, blob, sizeof(blob) - 1);
        if (length == 0) {
            Serial.printf("combos: missing blob %u\n", i);
            continue;
        }
        blob[length] = '\0';
        Serial.printf("combos: loading %u bytes '%s'\n", i, blob);

        const char* error = nullptr;
        Combo combo;
        if (!decodeCombo(blob, combo, &error)) {
            Serial.printf("combos: dropped saved combo %u (%s)\n", i, error != nullptr ? error : "invalid");
            continue;
        }
        printCombo("built", comboCount, combo);
        combos[comboCount++] = combo;
    }

    prefs.end();
    Serial.printf("combos: loaded %u from NVS\n", comboCount);
}

void comboBegin() {
    loadAll();
}

uint8_t comboStoredCount() {
    return comboCount;
}

String comboListJson() {
    String json;
    json.reserve(96 + comboCount * 160);
    json += "{\"max\":";
    json += COMBO_SLOT_COUNT;
    json += ",\"chainMax\":";
    json += COMBO_KEY_MAX;
    json += ",\"holdMax\":";
    json += COMBO_HOLD_MAX;
    json += ",\"combos\":[";

    for (uint8_t i = 0; i < comboCount; ++i) {
        if (i > 0) {
            json += ',';
        }
        const Combo& combo = combos[i];
        json += "{\"index\":";
        json += i;
        json += ",\"name\":\"";
        json += jsonEscape(combo.name);
        json += "\",\"hold\":";
        json += combo.hold ? "true" : "false";
        json += ",\"holdTime\":";
        json += combo.timeToHold;
        json += ",\"waitAfterTap\":";
        json += combo.timeToWaitAfterTap;
        json += ",\"keys\":[";
        for (uint8_t k = 0; k < combo.count; ++k) {
            if (k > 0) {
                json += ',';
            }
            const KeyCatalogEntry* entry = keymapCatalogEntry(combo.steps[k]);
            json += "{\"id\":\"";
            json += jsonEscape(entry != nullptr ? entry->id : "");
            json += "\"}";
        }
        json += "]}";
    }

    json += "]}";
    return json;
}

bool comboAdd(const char* name, bool hold, const char* keys, const char* holdTime, const char* waitAfterTap, const char** error) {
    if (error == nullptr) {
        return false;
    }
    *error = "save_failed";
    if (comboCount >= COMBO_SLOT_COUNT) {
        *error = "full";
        return false;
    }

    Combo combo;
    if (!fillCombo(combo, name, hold, keys, holdTime, waitAfterTap, false, error)) {
        return false;
    }

    combos[comboCount] = combo;
    comboCount++;
    if (!saveAll()) {
        comboCount--;
        memset(&combos[comboCount], 0, sizeof(Combo));
        *error = "save_failed";
        return false;
    }
    return true;
}

bool comboDelete(uint8_t index) {
    if (index >= comboCount) {
        return false;
    }

    Combo removed = combos[index];
    for (uint8_t i = index + 1; i < comboCount; ++i) {
        combos[i - 1] = combos[i];
    }
    comboCount--;
    memset(&combos[comboCount], 0, sizeof(Combo));

    if (!saveAll()) {
        for (uint8_t i = comboCount; i > index; --i) {
            combos[i] = combos[i - 1];
        }
        combos[index] = removed;
        comboCount++;
        return false;
    }
    return true;
}

bool comboFire(uint8_t index) {
    if (index >= comboCount) {
        return false;
    }

    const Combo& combo = combos[index];
    if (combo.count == 0 || !queueHasRoom(combo.count)) {
        return false;
    }

    uint16_t codes[COMBO_KEY_MAX];
    KeyKind kind = combo.kind;
    for (uint8_t i = 0; i < combo.count; ++i) {
        const KeyCatalogEntry* entry = keymapCatalogEntry(combo.steps[i]);
        if (entry == nullptr) {
            return false;
        }
        codes[i] = entry->code;
        kind = entry->kind;
    }

    printCombo("fire", index, combo);
    return queuePushKeys(codes, combo.count, kind, combo.timeToHold, combo.timeToWaitAfterTap, combo.hold);
}
