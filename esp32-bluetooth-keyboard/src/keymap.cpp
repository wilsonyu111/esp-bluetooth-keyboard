#include "keymap.h"
#include "messageQueue.h"
#include <HijelHID_BLEKeyboard.h>
#include <Preferences.h>

static Preferences prefs;
static KeySlot slots[KEYMAP_SLOT_COUNT];

#define K_ENTRY(id, label, group, kind, code) \
    { #id, label, group, kind, (uint16_t)(code) }

static const KeyCatalogEntry CATALOG[] = {
    // Media — Playback
    K_ENTRY(MEDIA_PLAY, "Play", "Media Playback", KeyKind::Media, MEDIA_PLAY),
    K_ENTRY(MEDIA_PAUSE, "Pause", "Media Playback", KeyKind::Media, MEDIA_PAUSE),
    K_ENTRY(MEDIA_RECORD, "Record", "Media Playback", KeyKind::Media, MEDIA_RECORD),
    K_ENTRY(MEDIA_FAST_FORWARD, "Fast Forward", "Media Playback", KeyKind::Media, MEDIA_FAST_FORWARD),
    K_ENTRY(MEDIA_REWIND, "Rewind", "Media Playback", KeyKind::Media, MEDIA_REWIND),
    K_ENTRY(MEDIA_NEXT_TRACK, "Next Track", "Media Playback", KeyKind::Media, MEDIA_NEXT_TRACK),
    K_ENTRY(MEDIA_PREV_TRACK, "Previous Track", "Media Playback", KeyKind::Media, MEDIA_PREV_TRACK),
    K_ENTRY(MEDIA_STOP, "Stop", "Media Playback", KeyKind::Media, MEDIA_STOP),
    K_ENTRY(MEDIA_EJECT, "Eject", "Media Playback", KeyKind::Media, MEDIA_EJECT),
    K_ENTRY(MEDIA_RANDOM_PLAY, "Shuffle", "Media Playback", KeyKind::Media, MEDIA_RANDOM_PLAY),
    K_ENTRY(MEDIA_PLAY_PAUSE, "Play / Pause", "Media Playback", KeyKind::Media, MEDIA_PLAY_PAUSE),
    K_ENTRY(MEDIA_PLAY_SKIP, "Play Skip", "Media Playback", KeyKind::Media, MEDIA_PLAY_SKIP),

    // Media — Volume
    K_ENTRY(MEDIA_MUTE, "Mute", "Media Volume", KeyKind::Media, MEDIA_MUTE),
    K_ENTRY(MEDIA_BASS_BOOST, "Bass Boost", "Media Volume", KeyKind::Media, MEDIA_BASS_BOOST),
    K_ENTRY(MEDIA_VOLUME_UP, "Volume Up", "Media Volume", KeyKind::Media, MEDIA_VOLUME_UP),
    K_ENTRY(MEDIA_VOLUME_DOWN, "Volume Down", "Media Volume", KeyKind::Media, MEDIA_VOLUME_DOWN),
    K_ENTRY(MEDIA_BASS_UP, "Bass Up", "Media Volume", KeyKind::Media, MEDIA_BASS_UP),
    K_ENTRY(MEDIA_BASS_DOWN, "Bass Down", "Media Volume", KeyKind::Media, MEDIA_BASS_DOWN),
    K_ENTRY(MEDIA_TREBLE_UP, "Treble Up", "Media Volume", KeyKind::Media, MEDIA_TREBLE_UP),
    K_ENTRY(MEDIA_TREBLE_DOWN, "Treble Down", "Media Volume", KeyKind::Media, MEDIA_TREBLE_DOWN),

    // Media — Display
    K_ENTRY(MEDIA_BRIGHTNESS_UP, "Brightness Up", "Media Display", KeyKind::Media, MEDIA_BRIGHTNESS_UP),
    K_ENTRY(MEDIA_BRIGHTNESS_DOWN, "Brightness Down", "Media Display", KeyKind::Media, MEDIA_BRIGHTNESS_DOWN),
    K_ENTRY(MEDIA_DISPLAY_BACKLIGHT, "Display Backlight", "Media Display", KeyKind::Media, MEDIA_DISPLAY_BACKLIGHT),
    K_ENTRY(MEDIA_KBD_BACKLIGHT_DOWN, "Keyboard Backlight Down", "Media Display", KeyKind::Media, MEDIA_KBD_BACKLIGHT_DOWN),
    K_ENTRY(MEDIA_KBD_BACKLIGHT_UP, "Keyboard Backlight Up", "Media Display", KeyKind::Media, MEDIA_KBD_BACKLIGHT_UP),
    K_ENTRY(MEDIA_KBD_BACKLIGHT_TOGGLE, "Keyboard Backlight Toggle", "Media Display", KeyKind::Media, MEDIA_KBD_BACKLIGHT_TOGGLE),

    // Media — App launch
    K_ENTRY(MEDIA_MEDIA_SELECT, "Media Select", "Media Apps", KeyKind::Media, MEDIA_MEDIA_SELECT),
    K_ENTRY(MEDIA_MAIL, "Mail", "Media Apps", KeyKind::Media, MEDIA_MAIL),
    K_ENTRY(MEDIA_CALCULATOR, "Calculator", "Media Apps", KeyKind::Media, MEDIA_CALCULATOR),
    K_ENTRY(MEDIA_FILE_EXPLORER, "File Explorer", "Media Apps", KeyKind::Media, MEDIA_FILE_EXPLORER),
    K_ENTRY(MEDIA_SCREENSAVER, "Screensaver", "Media Apps", KeyKind::Media, MEDIA_SCREENSAVER),
    K_ENTRY(MEDIA_TASK_MANAGER, "Task Manager", "Media Apps", KeyKind::Media, MEDIA_TASK_MANAGER),
    K_ENTRY(MEDIA_LOCK_SCREEN, "Lock Screen", "Media Apps", KeyKind::Media, MEDIA_LOCK_SCREEN),

    // Media — Browser
    K_ENTRY(MEDIA_BROWSER_SEARCH, "Browser Search", "Media Browser", KeyKind::Media, MEDIA_BROWSER_SEARCH),
    K_ENTRY(MEDIA_BROWSER_HOME, "Browser Home", "Media Browser", KeyKind::Media, MEDIA_BROWSER_HOME),
    K_ENTRY(MEDIA_BROWSER_BACK, "Browser Back", "Media Browser", KeyKind::Media, MEDIA_BROWSER_BACK),
    K_ENTRY(MEDIA_BROWSER_FORWARD, "Browser Forward", "Media Browser", KeyKind::Media, MEDIA_BROWSER_FORWARD),
    K_ENTRY(MEDIA_BROWSER_STOP, "Browser Stop", "Media Browser", KeyKind::Media, MEDIA_BROWSER_STOP),
    K_ENTRY(MEDIA_BROWSER_REFRESH, "Browser Refresh", "Media Browser", KeyKind::Media, MEDIA_BROWSER_REFRESH),
    K_ENTRY(MEDIA_BROWSER_BOOKMARKS, "Browser Bookmarks", "Media Browser", KeyKind::Media, MEDIA_BROWSER_BOOKMARKS),

    // Media — Power
    K_ENTRY(MEDIA_SLEEP, "Sleep", "Media Power", KeyKind::Media, MEDIA_SLEEP),

    // Keyboard — Letters
    K_ENTRY(KEY_A, "A", "Letters", KeyKind::Keyboard, KEY_A),
    K_ENTRY(KEY_B, "B", "Letters", KeyKind::Keyboard, KEY_B),
    K_ENTRY(KEY_C, "C", "Letters", KeyKind::Keyboard, KEY_C),
    K_ENTRY(KEY_D, "D", "Letters", KeyKind::Keyboard, KEY_D),
    K_ENTRY(KEY_E, "E", "Letters", KeyKind::Keyboard, KEY_E),
    K_ENTRY(KEY_F, "F", "Letters", KeyKind::Keyboard, KEY_F),
    K_ENTRY(KEY_G, "G", "Letters", KeyKind::Keyboard, KEY_G),
    K_ENTRY(KEY_H, "H", "Letters", KeyKind::Keyboard, KEY_H),
    K_ENTRY(KEY_I, "I", "Letters", KeyKind::Keyboard, KEY_I),
    K_ENTRY(KEY_J, "J", "Letters", KeyKind::Keyboard, KEY_J),
    K_ENTRY(KEY_K, "K", "Letters", KeyKind::Keyboard, KEY_K),
    K_ENTRY(KEY_L, "L", "Letters", KeyKind::Keyboard, KEY_L),
    K_ENTRY(KEY_M, "M", "Letters", KeyKind::Keyboard, KEY_M),
    K_ENTRY(KEY_N, "N", "Letters", KeyKind::Keyboard, KEY_N),
    K_ENTRY(KEY_O, "O", "Letters", KeyKind::Keyboard, KEY_O),
    K_ENTRY(KEY_P, "P", "Letters", KeyKind::Keyboard, KEY_P),
    K_ENTRY(KEY_Q, "Q", "Letters", KeyKind::Keyboard, KEY_Q),
    K_ENTRY(KEY_R, "R", "Letters", KeyKind::Keyboard, KEY_R),
    K_ENTRY(KEY_S, "S", "Letters", KeyKind::Keyboard, KEY_S),
    K_ENTRY(KEY_T, "T", "Letters", KeyKind::Keyboard, KEY_T),
    K_ENTRY(KEY_U, "U", "Letters", KeyKind::Keyboard, KEY_U),
    K_ENTRY(KEY_V, "V", "Letters", KeyKind::Keyboard, KEY_V),
    K_ENTRY(KEY_W, "W", "Letters", KeyKind::Keyboard, KEY_W),
    K_ENTRY(KEY_X, "X", "Letters", KeyKind::Keyboard, KEY_X),
    K_ENTRY(KEY_Y, "Y", "Letters", KeyKind::Keyboard, KEY_Y),
    K_ENTRY(KEY_Z, "Z", "Letters", KeyKind::Keyboard, KEY_Z),

    // Keyboard — Digits
    K_ENTRY(KEY_1, "1", "Digits", KeyKind::Keyboard, KEY_1),
    K_ENTRY(KEY_2, "2", "Digits", KeyKind::Keyboard, KEY_2),
    K_ENTRY(KEY_3, "3", "Digits", KeyKind::Keyboard, KEY_3),
    K_ENTRY(KEY_4, "4", "Digits", KeyKind::Keyboard, KEY_4),
    K_ENTRY(KEY_5, "5", "Digits", KeyKind::Keyboard, KEY_5),
    K_ENTRY(KEY_6, "6", "Digits", KeyKind::Keyboard, KEY_6),
    K_ENTRY(KEY_7, "7", "Digits", KeyKind::Keyboard, KEY_7),
    K_ENTRY(KEY_8, "8", "Digits", KeyKind::Keyboard, KEY_8),
    K_ENTRY(KEY_9, "9", "Digits", KeyKind::Keyboard, KEY_9),
    K_ENTRY(KEY_0, "0", "Digits", KeyKind::Keyboard, KEY_0),

    // Keyboard — Editing
    K_ENTRY(KEY_RETURN, "Enter", "Editing", KeyKind::Keyboard, KEY_RETURN),
    K_ENTRY(KEY_ESCAPE, "Escape", "Editing", KeyKind::Keyboard, KEY_ESCAPE),
    K_ENTRY(KEY_BACKSPACE, "Backspace", "Editing", KeyKind::Keyboard, KEY_BACKSPACE),
    K_ENTRY(KEY_TAB, "Tab", "Editing", KeyKind::Keyboard, KEY_TAB),
    K_ENTRY(KEY_SPACE, "Space", "Editing", KeyKind::Keyboard, KEY_SPACE),

    // Keyboard — Punctuation
    K_ENTRY(KEY_MINUS, "- / _", "Punctuation", KeyKind::Keyboard, KEY_MINUS),
    K_ENTRY(KEY_EQUAL, "= / +", "Punctuation", KeyKind::Keyboard, KEY_EQUAL),
    K_ENTRY(KEY_LEFTBRACE, "[ / {", "Punctuation", KeyKind::Keyboard, KEY_LEFTBRACE),
    K_ENTRY(KEY_RIGHTBRACE, "] / }", "Punctuation", KeyKind::Keyboard, KEY_RIGHTBRACE),
    K_ENTRY(KEY_BACKSLASH, "\\ / |", "Punctuation", KeyKind::Keyboard, KEY_BACKSLASH),
    K_ENTRY(KEY_HASH_TILDE, "# / ~ (Non-US)", "Punctuation", KeyKind::Keyboard, KEY_HASH_TILDE),
    K_ENTRY(KEY_SEMICOLON, "; / :", "Punctuation", KeyKind::Keyboard, KEY_SEMICOLON),
    K_ENTRY(KEY_APOSTROPHE, "' / \"", "Punctuation", KeyKind::Keyboard, KEY_APOSTROPHE),
    K_ENTRY(KEY_GRAVE, "` / ~", "Punctuation", KeyKind::Keyboard, KEY_GRAVE),
    K_ENTRY(KEY_COMMA, ", / <", "Punctuation", KeyKind::Keyboard, KEY_COMMA),
    K_ENTRY(KEY_DOT, ". / >", "Punctuation", KeyKind::Keyboard, KEY_DOT),
    K_ENTRY(KEY_SLASH, "/ / ?", "Punctuation", KeyKind::Keyboard, KEY_SLASH),

    // Keyboard — Locks
    K_ENTRY(KEY_CAPS_LOCK, "Caps Lock", "Locks", KeyKind::Keyboard, KEY_CAPS_LOCK),
    K_ENTRY(KEY_SCROLL_LOCK, "Scroll Lock", "Locks", KeyKind::Keyboard, KEY_SCROLL_LOCK),
    K_ENTRY(KEY_NUM_LOCK, "Num Lock", "Locks", KeyKind::Keyboard, KEY_NUM_LOCK),

    // Keyboard — Function
    K_ENTRY(KEY_F1, "F1", "Function", KeyKind::Keyboard, KEY_F1),
    K_ENTRY(KEY_F2, "F2", "Function", KeyKind::Keyboard, KEY_F2),
    K_ENTRY(KEY_F3, "F3", "Function", KeyKind::Keyboard, KEY_F3),
    K_ENTRY(KEY_F4, "F4", "Function", KeyKind::Keyboard, KEY_F4),
    K_ENTRY(KEY_F5, "F5", "Function", KeyKind::Keyboard, KEY_F5),
    K_ENTRY(KEY_F6, "F6", "Function", KeyKind::Keyboard, KEY_F6),
    K_ENTRY(KEY_F7, "F7", "Function", KeyKind::Keyboard, KEY_F7),
    K_ENTRY(KEY_F8, "F8", "Function", KeyKind::Keyboard, KEY_F8),
    K_ENTRY(KEY_F9, "F9", "Function", KeyKind::Keyboard, KEY_F9),
    K_ENTRY(KEY_F10, "F10", "Function", KeyKind::Keyboard, KEY_F10),
    K_ENTRY(KEY_F11, "F11", "Function", KeyKind::Keyboard, KEY_F11),
    K_ENTRY(KEY_F12, "F12", "Function", KeyKind::Keyboard, KEY_F12),
    K_ENTRY(KEY_F13, "F13", "Function", KeyKind::Keyboard, KEY_F13),
    K_ENTRY(KEY_F14, "F14", "Function", KeyKind::Keyboard, KEY_F14),
    K_ENTRY(KEY_F15, "F15", "Function", KeyKind::Keyboard, KEY_F15),
    K_ENTRY(KEY_F16, "F16", "Function", KeyKind::Keyboard, KEY_F16),
    K_ENTRY(KEY_F17, "F17", "Function", KeyKind::Keyboard, KEY_F17),
    K_ENTRY(KEY_F18, "F18", "Function", KeyKind::Keyboard, KEY_F18),
    K_ENTRY(KEY_F19, "F19", "Function", KeyKind::Keyboard, KEY_F19),
    K_ENTRY(KEY_F20, "F20", "Function", KeyKind::Keyboard, KEY_F20),
    K_ENTRY(KEY_F21, "F21", "Function", KeyKind::Keyboard, KEY_F21),
    K_ENTRY(KEY_F22, "F22", "Function", KeyKind::Keyboard, KEY_F22),
    K_ENTRY(KEY_F23, "F23", "Function", KeyKind::Keyboard, KEY_F23),
    K_ENTRY(KEY_F24, "F24", "Function", KeyKind::Keyboard, KEY_F24),

    // Keyboard — Navigation / System
    K_ENTRY(KEY_PRINT_SCREEN, "Print Screen", "Navigation", KeyKind::Keyboard, KEY_PRINT_SCREEN),
    K_ENTRY(KEY_PAUSE, "Pause", "Navigation", KeyKind::Keyboard, KEY_PAUSE),
    K_ENTRY(KEY_INSERT, "Insert", "Navigation", KeyKind::Keyboard, KEY_INSERT),
    K_ENTRY(KEY_HOME, "Home", "Navigation", KeyKind::Keyboard, KEY_HOME),
    K_ENTRY(KEY_PAGE_UP, "Page Up", "Navigation", KeyKind::Keyboard, KEY_PAGE_UP),
    K_ENTRY(KEY_DELETE, "Delete", "Navigation", KeyKind::Keyboard, KEY_DELETE),
    K_ENTRY(KEY_END, "End", "Navigation", KeyKind::Keyboard, KEY_END),
    K_ENTRY(KEY_PAGE_DOWN, "Page Down", "Navigation", KeyKind::Keyboard, KEY_PAGE_DOWN),
    K_ENTRY(KEY_RIGHT, "Right", "Navigation", KeyKind::Keyboard, KEY_RIGHT),
    K_ENTRY(KEY_LEFT, "Left", "Navigation", KeyKind::Keyboard, KEY_LEFT),
    K_ENTRY(KEY_DOWN, "Down", "Navigation", KeyKind::Keyboard, KEY_DOWN),
    K_ENTRY(KEY_UP, "Up", "Navigation", KeyKind::Keyboard, KEY_UP),

    // Keyboard — Numpad
    K_ENTRY(KEY_KP_SLASH, "Keypad /", "Numpad", KeyKind::Keyboard, KEY_KP_SLASH),
    K_ENTRY(KEY_KP_ASTERISK, "Keypad *", "Numpad", KeyKind::Keyboard, KEY_KP_ASTERISK),
    K_ENTRY(KEY_KP_MINUS, "Keypad -", "Numpad", KeyKind::Keyboard, KEY_KP_MINUS),
    K_ENTRY(KEY_KP_PLUS, "Keypad +", "Numpad", KeyKind::Keyboard, KEY_KP_PLUS),
    K_ENTRY(KEY_KP_ENTER, "Keypad Enter", "Numpad", KeyKind::Keyboard, KEY_KP_ENTER),
    K_ENTRY(KEY_KP_1, "Keypad 1", "Numpad", KeyKind::Keyboard, KEY_KP_1),
    K_ENTRY(KEY_KP_2, "Keypad 2", "Numpad", KeyKind::Keyboard, KEY_KP_2),
    K_ENTRY(KEY_KP_3, "Keypad 3", "Numpad", KeyKind::Keyboard, KEY_KP_3),
    K_ENTRY(KEY_KP_4, "Keypad 4", "Numpad", KeyKind::Keyboard, KEY_KP_4),
    K_ENTRY(KEY_KP_5, "Keypad 5", "Numpad", KeyKind::Keyboard, KEY_KP_5),
    K_ENTRY(KEY_KP_6, "Keypad 6", "Numpad", KeyKind::Keyboard, KEY_KP_6),
    K_ENTRY(KEY_KP_7, "Keypad 7", "Numpad", KeyKind::Keyboard, KEY_KP_7),
    K_ENTRY(KEY_KP_8, "Keypad 8", "Numpad", KeyKind::Keyboard, KEY_KP_8),
    K_ENTRY(KEY_KP_9, "Keypad 9", "Numpad", KeyKind::Keyboard, KEY_KP_9),
    K_ENTRY(KEY_KP_0, "Keypad 0", "Numpad", KeyKind::Keyboard, KEY_KP_0),
    K_ENTRY(KEY_KP_DOT, "Keypad .", "Numpad", KeyKind::Keyboard, KEY_KP_DOT),
    K_ENTRY(KEY_KP_EQUAL, "Keypad =", "Numpad", KeyKind::Keyboard, KEY_KP_EQUAL),
    K_ENTRY(KEY_KP_COMMA, "Keypad ,", "Numpad", KeyKind::Keyboard, KEY_KP_COMMA),

    // Keyboard — Extra
    K_ENTRY(KEY_NONUS_BACKSLASH, "Non-US \\", "Extra", KeyKind::Keyboard, KEY_NONUS_BACKSLASH),
    K_ENTRY(KEY_APPLICATION, "Application / Menu", "Extra", KeyKind::Keyboard, KEY_APPLICATION),
    K_ENTRY(KEY_POWER, "Power", "Extra", KeyKind::Keyboard, KEY_POWER),

    // Keyboard — International
    K_ENTRY(KEY_INTL1, "Intl 1", "International", KeyKind::Keyboard, KEY_INTL1),
    K_ENTRY(KEY_INTL2, "Intl 2", "International", KeyKind::Keyboard, KEY_INTL2),
    K_ENTRY(KEY_INTL3, "Intl 3", "International", KeyKind::Keyboard, KEY_INTL3),
    K_ENTRY(KEY_INTL4, "Intl 4", "International", KeyKind::Keyboard, KEY_INTL4),
    K_ENTRY(KEY_INTL5, "Intl 5", "International", KeyKind::Keyboard, KEY_INTL5),
    K_ENTRY(KEY_INTL6, "Intl 6", "International", KeyKind::Keyboard, KEY_INTL6),

    // Keyboard — Language
    K_ENTRY(KEY_LANG1, "Lang 1", "Language", KeyKind::Keyboard, KEY_LANG1),
    K_ENTRY(KEY_LANG2, "Lang 2", "Language", KeyKind::Keyboard, KEY_LANG2),
    K_ENTRY(KEY_LANG3, "Lang 3", "Language", KeyKind::Keyboard, KEY_LANG3),
    K_ENTRY(KEY_LANG4, "Lang 4", "Language", KeyKind::Keyboard, KEY_LANG4),
    K_ENTRY(KEY_LANG5, "Lang 5", "Language", KeyKind::Keyboard, KEY_LANG5),

    // Keyboard — Modifier keycodes
    K_ENTRY(KEY_LCTRL, "Left Ctrl", "Modifiers", KeyKind::Keyboard, KEY_LCTRL),
    K_ENTRY(KEY_LSHIFT, "Left Shift", "Modifiers", KeyKind::Keyboard, KEY_LSHIFT),
    K_ENTRY(KEY_LALT, "Left Alt", "Modifiers", KeyKind::Keyboard, KEY_LALT),
    K_ENTRY(KEY_LGUI, "Left Win/Cmd", "Modifiers", KeyKind::Keyboard, KEY_LGUI),
    K_ENTRY(KEY_RCTRL, "Right Ctrl", "Modifiers", KeyKind::Keyboard, KEY_RCTRL),
    K_ENTRY(KEY_RSHIFT, "Right Shift", "Modifiers", KeyKind::Keyboard, KEY_RSHIFT),
    K_ENTRY(KEY_RALT, "Right Alt", "Modifiers", KeyKind::Keyboard, KEY_RALT),
    K_ENTRY(KEY_RGUI, "Right Win/Cmd", "Modifiers", KeyKind::Keyboard, KEY_RGUI),
};

static const uint16_t CATALOG_COUNT = sizeof(CATALOG) / sizeof(CATALOG[0]);

static const char* DEFAULT_IDS[KEYMAP_SLOT_COUNT] = {
    "MEDIA_STOP",
    "MEDIA_PREV_TRACK",
    "MEDIA_PLAY_PAUSE",
    "MEDIA_NEXT_TRACK",
    "MEDIA_MUTE",
    "MEDIA_VOLUME_DOWN",
    "MEDIA_VOLUME_UP",
};

static String jsonEscape(const char* text) {
    String out;
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

static String formatSlotLabel(const KeyCatalogEntry& entry, uint8_t modifiers) {
    (void)modifiers;
    return String(entry.label);
}

static void applyDefaults() {
    for (uint8_t i = 0; i < KEYMAP_SLOT_COUNT; ++i) {
        int idx = keymapFindById(DEFAULT_IDS[i]);
        slots[i].catalogIndex = idx >= 0 ? (uint16_t)idx : 0;
        slots[i].modifiers = 0;
    }
}

uint16_t keymapCatalogCount() {
    return CATALOG_COUNT;
}

const KeyCatalogEntry* keymapCatalogEntry(uint16_t index) {
    if (index >= CATALOG_COUNT) {
        return nullptr;
    }
    return &CATALOG[index];
}

int keymapFindById(const char* id) {
    if (id == nullptr) {
        return -1;
    }
    for (uint16_t i = 0; i < CATALOG_COUNT; ++i) {
        if (strcmp(CATALOG[i].id, id) == 0) {
            return (int)i;
        }
    }
    return -1;
}

const KeySlot* keymapSlots() {
    return slots;
}

bool keymapSetSlot(uint8_t slot, const char* actionId, uint8_t modifiers) {
    if (slot >= KEYMAP_SLOT_COUNT) {
        return false;
    }
    int idx = keymapFindById(actionId);
    if (idx < 0) {
        return false;
    }

    const KeyCatalogEntry& entry = CATALOG[idx];
    if (entry.kind == KeyKind::Media) {
        modifiers = 0;
    }

    slots[slot].catalogIndex = (uint16_t)idx;
    slots[slot].modifiers = modifiers;
    return true;
}

bool keymapSave() {
    if (!prefs.begin("keymap", false)) {
        Serial.println("keymap: failed to open Preferences for write");
        return false;
    }

    for (uint8_t i = 0; i < KEYMAP_SLOT_COUNT; ++i) {
        char idKey[4];
        char modKey[4];
        snprintf(idKey, sizeof(idKey), "s%u", i);
        snprintf(modKey, sizeof(modKey), "m%u", i);
        const KeyCatalogEntry& entry = CATALOG[slots[i].catalogIndex];
        prefs.putString(idKey, entry.id);
        prefs.putUChar(modKey, slots[i].modifiers);
    }

    prefs.end();
    Serial.println("keymap: saved to NVS");
    return true;
}

bool keymapResetSlot(uint8_t slot) {
    if (slot >= KEYMAP_SLOT_COUNT) {
        return false;
    }

    int idx = keymapFindById(DEFAULT_IDS[slot]);
    slots[slot].catalogIndex = idx >= 0 ? (uint16_t)idx : 0;
    slots[slot].modifiers = 0;
    Serial.printf("keymap: reset slot %u to %s\n", slot, DEFAULT_IDS[slot]);
    return keymapSave();
}

bool keymapResetToDefaults() {
    applyDefaults();
    Serial.println("keymap: reset to media defaults");
    return keymapSave();
}

void keymapLoad() {
    applyDefaults();

    if (!prefs.begin("keymap", true)) {
        Serial.println("keymap: no saved map, using defaults");
        return;
    }

    for (uint8_t i = 0; i < KEYMAP_SLOT_COUNT; ++i) {
        char idKey[4];
        char modKey[4];
        snprintf(idKey, sizeof(idKey), "s%u", i);
        snprintf(modKey, sizeof(modKey), "m%u", i);

        String id = prefs.getString(idKey, "");
        if (id.length() == 0) {
            continue;
        }

        int idx = keymapFindById(id.c_str());
        if (idx < 0) {
            Serial.printf("keymap: unknown id '%s' in slot %u, keeping default\n", id.c_str(), i);
            continue;
        }

        uint8_t modifiers = prefs.getUChar(modKey, 0);
        if (CATALOG[idx].kind == KeyKind::Media) {
            modifiers = 0;
        }

        slots[i].catalogIndex = (uint16_t)idx;
        slots[i].modifiers = modifiers;
    }

    prefs.end();
    Serial.println("keymap: loaded from NVS");
}

void keymapBegin() {
    keymapLoad();
}

String keymapCatalogJson() {
    String json;
    json.reserve(CATALOG_COUNT * 64 + 32);
    json += "{\"keys\":[";

    for (uint16_t i = 0; i < CATALOG_COUNT; ++i) {
        if (i > 0) {
            json += ',';
        }
        const KeyCatalogEntry& e = CATALOG[i];
        json += "{\"id\":\"";
        json += jsonEscape(e.id);
        json += "\",\"label\":\"";
        json += jsonEscape(e.label);
        json += "\",\"group\":\"";
        json += jsonEscape(e.group);
        json += "\",\"kind\":\"";
        json += (e.kind == KeyKind::Media) ? "media" : "keyboard";
        json += "\"}";
    }

    json += "]}";
    return json;
}

String keymapSlotsJson() {
    String json;
    json.reserve(512);
    json += "{\"slots\":[";

    for (uint8_t i = 0; i < KEYMAP_SLOT_COUNT; ++i) {
        if (i > 0) {
            json += ',';
        }
        const KeySlot& slot = slots[i];
        const KeyCatalogEntry& entry = CATALOG[slot.catalogIndex];
        String label = formatSlotLabel(entry, slot.modifiers);

        json += "{\"slot\":";
        json += i;
        json += ",\"id\":\"";
        json += jsonEscape(entry.id);
        json += "\",\"label\":\"";
        json += jsonEscape(label.c_str());
        json += "\",\"kind\":\"";
        json += (entry.kind == KeyKind::Media) ? "media" : "keyboard";
        json += "\",\"modifiers\":";
        json += slot.modifiers;
        json += '}';
    }

    json += "]}";
    return json;
}

bool keymapFireSlot(uint8_t slot) {
    if (slot >= KEYMAP_SLOT_COUNT) {
        return false;
    }

    const KeySlot& mapping = slots[slot];
    if (mapping.catalogIndex >= CATALOG_COUNT) {
        return false;
    }

    const KeyCatalogEntry& entry = CATALOG[mapping.catalogIndex];
    uint16_t codes[QUEUE_CHORD_MAX];
    uint8_t count = 0;
    bool hold = false;

    if (entry.kind == KeyKind::Keyboard && mapping.modifiers != 0) {
        static const uint8_t bits[] = {
            KEY_MOD_LCTRL, KEY_MOD_LSHIFT, KEY_MOD_LALT, KEY_MOD_LGUI,
            KEY_MOD_RCTRL, KEY_MOD_RSHIFT, KEY_MOD_RALT, KEY_MOD_RGUI
        };
        static const uint16_t keys[] = {
            KEY_LCTRL, KEY_LSHIFT, KEY_LALT, KEY_LGUI,
            KEY_RCTRL, KEY_RSHIFT, KEY_RALT, KEY_RGUI
        };
        for (uint8_t i = 0; i < 8; ++i) {
            if ((mapping.modifiers & bits[i]) != 0 && count < QUEUE_CHORD_MAX) {
                codes[count++] = keys[i];
            }
        }
        hold = true;
    }

    if (count < QUEUE_CHORD_MAX) {
        codes[count++] = entry.code;
    }
    return queuePushKeys(codes, count, entry.kind, HID_DEFAULT_TAP_DELAY_MS, HID_DEFAULT_KEY_GAP_MS, hold);
}
