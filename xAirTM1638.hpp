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
    /*strcpy(names[2], "Saxophon");
    strcpy(names[1], "Posaune1");
    strcpy(names[0], "Vox3");
    strcpy(names[3], "Vox 2");*/
  };
  // states
  uint8_t st_mute = 0;
  uint16_t mutes = 0;
  uint8_t button = 0, last_button = 0;
  char name[13] = "Initial";
  // tasking cooldown
  unsigned long time_last = 0;
  // midi data
  // void setup();
  void update() {
#ifdef BRIGHTNESS_PIN
    tm->brightness(analogRead(BRIGHTNESS_PIN) / (128.0) - 1);
#endif
    button = tm->ReadKey16();
    if (button > 0) {
      if (button != last_button) {
        time_last = millis();
        st_mute = 0;
        getName(button - 1);
      }
      if (st_mute < 3) {
        if (stateMute() > st_mute) {
          st_mute = stateMute();
          switch (st_mute) {
            case 1:
              visualizeOnOff(button - 1);
              break;
            case 2:
              visualizeName(button - 1);
              break;
            default:
              visualizeMuteLeds();
          }
        }
      }
    } else {
      // button_up
      if (last_button > 0) {
        if (st_mute < 3) {
          mute(last_button - 1);
        }
        visualizeMuteLeds();
      }
    }
    last_button = button;
  };

  void visualizeOnOff(uint8_t id) {
    char buf[9];
    if (getMuteValue(id)) {
      sprintf(buf, "On%6d", id + 1);
    } else {
      sprintf(buf, "Off%5d", id + 1);
    }
    DEBUG_PRINTLN(buf);
    tm->DisplayStr(buf, 0);
  };
  void visualizeName(uint8_t id) {
    DEBUG_PRINTLN(name);
    tm->DisplayStr(name, 0);
  };

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
  // display text
  void displayText(){

  };

  // input callbacks
  void mute(uint8_t id) {
    if (muteChannelCallback) return muteChannelCallback(id, getMuteValue(id));
  };
  void getName(uint8_t id) {
    if (getNameCallback) return getNameCallback(id);
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
  uint8_t stateMute() { return (millis() - time_last) / 1000 + 1; }
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