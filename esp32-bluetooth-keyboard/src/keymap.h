#ifndef KEYMAP_H
#define KEYMAP_H

#include <Arduino.h>

static const uint8_t KEYMAP_SLOT_COUNT = 7;

enum class KeyKind : uint8_t {
    Keyboard = 0,
    Media = 1,
    Text = 2
};

struct KeyCatalogEntry {
    const char* id;
    const char* label;
    const char* group;
    KeyKind kind;
    uint16_t code;
};

struct KeySlot {
    uint16_t catalogIndex;
    uint8_t modifiers;
};

void keymapBegin();
uint16_t keymapCatalogCount();
const KeyCatalogEntry* keymapCatalogEntry(uint16_t index);
int keymapFindById(const char* id);

const KeySlot* keymapSlots();
bool keymapSetSlot(uint8_t slot, const char* actionId, uint8_t modifiers);
bool keymapSave();
bool keymapResetSlot(uint8_t slot);
bool keymapResetToDefaults();
void keymapLoad();

String keymapCatalogJson();
String keymapSlotsJson();
bool keymapFireSlot(uint8_t slot);

#endif
