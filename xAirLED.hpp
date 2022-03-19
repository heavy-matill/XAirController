#pragma once

#define FASTLED_ALLOW_INTERRUPTS 0
#include <Arduino.h>
#include <FastLED.h>  //https://github.com/FastLED/FastLED

#define LED_PIN 4
#define BRIGHTNESS_PIN A0

#define COLOR_ORDER GRB
#define CHIPSET WS2811

#define BRIGHTNESS 32

#ifndef MY_DEBUG
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#else
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#endif

const uint8_t kMatrixWidth = 4;
const uint8_t kMatrixHeight = 4;
const bool kMatrixSerpentineLayout = true;

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

class XAirLED {
 public:
  XAirLED() {
    FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
        .setCorrection(TypicalSMD5050);
    FastLED.setBrightness(BRIGHTNESS);
    // fill_solid(leds, NUM_LEDS, CRGB::Blue);
    visualizeColors();
    FastLED.show();
  };
  // states
  uint16_t mutes = 0;
  // values [0…15] representing {OFF, RD, GN, YE, BL, MG, CY, WH, OFFi, RDi,
  // GNi, YEi, BLi, MGi, CYi, WHi}
  uint8_t colors[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  uint8_t st_show = 0;
  uint32_t time_last_input = millis();
  void update() {
    // update FastLED
    if (millis() - time_last_input > 60000) {
      uint8_t brightness = FastLED.getBrightness();
      if (brightness) {
        FastLED.setBrightness(--brightness);
        FastLED.show();
        FastLED.delay(1);
      }
      return;
      // otherwise dont display anything new
    }
    bool time_switch = (millis() % 1000) > 666;
    if (time_switch && st_show) {
      st_show = 0;
#ifdef BRIGHTNESS_PIN
      FastLED.setBrightness(max({16.0, analogRead(BRIGHTNESS_PIN) / (4.0)}));
#endif
      visualizeColors();
      visualizeMuteLeds();
      FastLED.show();
      FastLED.delay(1);

    } else {
      if (!time_switch && !st_show) {
        st_show = 1;
        visualizeColors();
        FastLED.show();
        FastLED.delay(1);
      }
    }
  };

  // setters to set local mixer values
  void setMuted(uint8_t id, bool val) {
    // note time of last input
    time_last_input = millis();
    setMuteBit(id, val);
    if (val) {
      leds[pos[id]] = CRGB::Red;
    } else {
      leds[pos[id]] = color_map[colors[id] % 8];
    }
    FastLED.show();
    FastLED.delay(1);
  };
  void setColor(uint8_t id, uint8_t val) { colors[id] = val; };

  // button visualizations
  void visualizeColors() {
    for (uint8_t id = 0; id < 16; id++) {
      leds[pos[id]] = color_map[colors[id] % 8] / 4.0;
    }
  };
  void visualizeMuteLeds() {
    for (uint8_t id = 0; id < 16; id++) {
      if (getMuteValue(id)) {
        leds[pos[id]] = CRGB::Red;
      }
    }
  };

 public:
 private:
  CRGB leds[NUM_LEDS];
  bool getMuteValue(uint8_t i) { return (mutes & (1 << i)) > 0; };
  void setMuteBit(uint8_t i, bool val) {
    if (val)  // set bit
      mutes |= (1 << i);
    else  // clear bit
      mutes &= ~(1 << i);
  };
  uint8_t XY(uint8_t x, uint8_t y) {
    uint8_t i;

    if (kMatrixSerpentineLayout == false) {
      i = (y * kMatrixWidth) + x;
    }

    if (kMatrixSerpentineLayout == true) {
      if (y & 0x01) {
        // Odd rows run backwards
        uint8_t reverseX = (kMatrixWidth - 1) - x;
        i = (y * kMatrixWidth) + reverseX;
      } else {
        // Even rows run forwards
        i = (y * kMatrixWidth) + x;
      }
    }

    return i;
  };
  uint8_t posFunc(uint8_t x) {
    uint8_t y = x / 4;
    if (y & 0x01) {
      return y * 4 + 4 - x % 4;
    } else {
      return x;
    }
  };
  uint8_t pos[16] = {0, 1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11, 15, 14, 13, 12};
  // {OFF, RD, GN, YE, BL, MG, CY, WH}
  CRGB color_map[8] = {CRGB::Black, CRGB::Red,     CRGB::Green, CRGB::Yellow,
                       CRGB::Blue,  CRGB::Magenta, CRGB::Cyan,  CRGB::White};
};