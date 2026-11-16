#include "status_led.h"
#include <Arduino.h>

#if defined(RGB_BUILTIN)
static bool s_rainbowActive = false;
static uint8_t s_rainbowHue = 0;
static uint32_t s_rainbowLastMs = 0;

static void hsvToRgb(uint8_t hue, uint8_t brightness, uint8_t* r, uint8_t* g, uint8_t* b) {
    const uint8_t region = hue / 43;
    const uint8_t remainder = (hue - (region * 43)) * 6;
    const uint8_t p = 0;
    const uint8_t q = (brightness * (255 - remainder)) >> 8;
    const uint8_t t = (brightness * remainder) >> 8;

    switch (region) {
        case 0:
            *r = brightness;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = brightness;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = brightness;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = brightness;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = brightness;
            break;
        default:
            *r = brightness;
            *g = p;
            *b = q;
            break;
    }
}
#endif

void statusLedBegin() {
#if defined(RGB_BUILTIN)
    s_rainbowActive = false;
    s_rainbowHue = 0;
    s_rainbowLastMs = 0;
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
#endif
}

void statusLedEnableRainbow() {
#if defined(RGB_BUILTIN)
    s_rainbowActive = true;
    s_rainbowHue = 0;
    s_rainbowLastMs = 0;
#endif
}

void statusLedService() {
#if defined(RGB_BUILTIN)
    if (!s_rainbowActive) {
        return;
    }

    const uint32_t now = millis();
    if (now - s_rainbowLastMs < 20) {
        return;
    }
    s_rainbowLastMs = now;

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    hsvToRgb(s_rainbowHue, RGB_BRIGHTNESS, &r, &g, &b);
    neopixelWrite(RGB_BUILTIN, r, g, b);
    s_rainbowHue++;
#endif
}
