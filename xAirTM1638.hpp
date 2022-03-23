#pragma once

#include <Arduino.h>
#include <TM1638plus_Model2.h>  //https://github.com/gavinlyonsrepo/TM1638plus

#ifndef MY_DEBUG
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#else
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#endif

#define BRIGHTNESS_PIN A0

class XAirTM1638 {
 public:
  XAirTM1638(uint8_t strobe = 2, uint8_t clock = 0, uint8_t dio = 16,
             bool swap_nibbles = false, bool high_freq = false) {
    tm = new TM1638plus_Model2(strobe, clock, dio, swap_nibbles, high_freq);
    tm->displayBegin();  // Init the module
  };
  // states
  uint8_t st_mute_visual = 0;
  uint16_t mutes = 0;
  uint8_t button = 0, last_button = 0;
  char name[13] = "--------";
  char buf[9] = "";
  char text_buf[9] = "";
  // times
  unsigned long time_text_end = millis();
  unsigned long time_last_button = millis();
  // midi data
  // void setup();
  void update() {
    button = tm->ReadKey16();
#ifdef BRIGHTNESS_PIN
    tm->brightness(analogRead(BRIGHTNESS_PIN) / (128.0) - 1);
#endif
    if (button > 0) {
      // button down
      if (button != last_button) {
        // new button
        time_last_button = millis();
        st_mute_visual = 0;
        // call to get name which may be needed if button held for 1 second
        getName(button - 1);
      }
      if (st_mute_visual < 3) {
        if (stateMute() > st_mute_visual) {
          st_mute_visual = stateMute();
          switch (st_mute_visual) {
            case 1:
              showOnOff(button - 1);
              break;
            case 2:
              showName();
              break;
          }
        }
      }
    } else {
      // no button
      if (last_button > 0) {
        // button_up
        if (st_mute_visual < 3) {
          mute(last_button - 1);
        }
      }
    }
    last_button = button;
    visualize();
  };

  void showOnOff(uint8_t id) {
    if (getMuteValue(id)) {
      sprintf(text_buf, "On%6d", id + 1);
    } else {
      sprintf(text_buf, "Off%5d", id + 1);
    }
    showStr();
  };
  void showName() { showStr(name); };

  void showStr(const char* text = nullptr, const uint32_t duration = 1000,
               const uint8_t dot_mask = 0) {
    time_text_end = millis() + duration;
    if (text) {
      DEBUG_PRINTLN(text);
      tm->DisplayStr(text, dot_mask);
    } else {
      DEBUG_PRINTLN(text_buf);
      tm->DisplayStr(text_buf, dot_mask);
    }
  };
  void showIP(const char* text, const uint32_t duration = 1000) {
    char text_buf[9] = "        ";
    uint8_t dot_mask = 0;
    int8_t j = strlen(text) - 1;
    for (int8_t i = 7; (i >= 0) && (j >= 0); j--) {
      if (text[j] == '.') {
        // check if character before this is a dot
        dot_mask |= (1 << (7 - i));
      } else {
        text_buf[i] = text[j];
        i--;
      }
    }
    return showStr(text_buf, duration, dot_mask);
  }
  bool visualizeTextFinished() { return millis() > time_text_end; };

  // setters to set local mixer values
  void setMuted(uint8_t id, bool val) {
    setMuteBit(id, val);
    visualizeMuteLeds();
  };
  void setName(uint8_t id, char* str) {
    if (button - 1 == id) {
      strncpy(name, str, 12);
      DEBUG_PRINTLN(name);
    }
  };

  // regular visualization, normally show mutes, unless overwritten with text
  void visualize() {
    if (visualizeTextFinished()) {
      visualizeMuteLeds();
    }
  }

  // button visualizations
  void visualizeMuteLeds() {
    tm->DisplaySegments(0, reverse(mutes));       // a vertical top
    tm->DisplaySegments(1, 0x00);                 // b horizontal top right
    tm->DisplaySegments(2, 0x00);                 // c horizontal bottom right
    tm->DisplaySegments(3, reverse(mutes >> 8));  // d vertical bottom
    tm->DisplaySegments(4, 0x00);                 // e horizontal top left
    tm->DisplaySegments(5, 0x00);                 // f horizontal bottom left
    tm->DisplaySegments(6, 0x00);                 // g vertical center
    tm->DisplaySegments(7, 0x00);                 // DP dot
  };

  // input callbacks
  void mute(uint8_t id) {
    if (muteChannelCallback) return muteChannelCallback(id, getMuteValue(id));
  };
  void getName(uint8_t id) {
    if (getNameCallback) {
      return getNameCallback(id);
    }
  };

  std::function<void(uint8_t, bool)> muteChannelCallback = nullptr;
  void registerMuteChannelCallback(std::function<void(uint8_t, bool)> cb) {
    muteChannelCallback = cb;
  };
  std::function<void(uint8_t)> getNameCallback = nullptr;
  void registerGetNameCallback(std::function<void(uint8_t)> cb) {
    getNameCallback = cb;
  };

 private:
  TM1638plus_Model2* tm;
  uint8_t stateMute() { return (millis() - time_last_button) / 1000 + 1; }
  bool getMuteValue(uint8_t i) { return (mutes & (1 << i)) > 0; };
  void setMuteBit(uint8_t i, bool val) {
    if (val)  // set bit
      mutes |= (1 << i);
    else  // clear bit
      mutes &= ~(1 << i);
  };
  uint16_t reverse(uint16_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
  };
};