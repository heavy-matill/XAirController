#pragma once

//#define ARDUINOOSC_DEBUGLOG_ENABLE
#define ARDUINOOSC_ENABLE_BUNDLE
#define ARDUINOOSC_MAX_MSG_BUNDLE_SIZE 138

// Please include ArduinoOSCWiFi.h to use ArduinoOSC on the platform
// which can use both WiFi and Ethernet
#include <ArduinoOSCWiFi.h>

#include <xTouchMiniMixer.hpp>

#ifndef MY_DEBUG
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// callback types
typedef std::function<void(uint8_t)> cb_u;
typedef std::function<void(uint8_t, uint8_t)> cb_uu;
typedef std::function<void(uint8_t, bool)> cb_ub;
typedef std::function<void(uint8_t, uint8_t, uint8_t)> cb_uuu;

class XAirController {
 public:
  XAirController(){};
  XAirController(char *new_host, uint16_t new_port) {
    host = new_host;
    port = new_port;
  };
  XAirController(char *new_host, uint16_t new_port,
                 XTouchMiniMixer *new_xTouch) {
    host = new_host;
    port = new_port;
    registerXTouch(new_xTouch);
  };
  void registerXTouch(XTouchMiniMixer *new_xTouch) {
    xTouch = new_xTouch;
    // register xAir callbacks in xTouch
    xTouch->registerMuteChannelCallback(
        [=](auto &&...args) { return muteChannel(args...); });
    xTouch->registerFadeChannelCallback(
        [=](auto &&...args) { return fadeChannel(args...); });
    xTouch->registerPanChannelCallback(
        [=](auto &&...args) { return panChannel(args...); });
    xTouch->registerGainChannelCallback(
        [=](auto &&...args) { return gainChannel(args...); });
    xTouch->registerFadeMainCallback(
        [=](auto &&...args) { return fadeMain(args...); });
    xTouch->registerFadeAuxCallback(
        [=](auto &&...args) { return fadeAux(args...); });

    // register xTouch callbacks in xAir
    registerOnFadeCallback(
        [=](auto &&...args) { return xTouch->setFaded(args...); });
    registerOnPanCallback(
        [=](auto &&...args) { return xTouch->setPanned(args...); });
    registerOnGainCallback(
        [=](auto &&...args) { return xTouch->setGained(args...); });
    registerOnMixCallback(
        [=](auto &&...args) { return xTouch->setMixed(args...); });
    registerOnAuxCallback(
        [=](auto &&...args) { return xTouch->setAuxFaded(args...); });
    registerOnMainCallback(
        [=](auto &&...args) { return xTouch->setMainFaded(args...); });
    registerOnMuteCallback(
        [=](auto &&...args) { return xTouch->setMuted(args...); });
    registerOnColorCallback(
        [=](auto &&...args) { return xTouch->setColored(args...); });
  }

  void setup() {
    // subscribe to init messages
    OscWiFi.subscribe(port, "/mutes",
                      [=](auto &&...args) { return onMutes(args...); });
    OscWiFi.subscribe(port, "/mutes",
                      [=](auto &&...args) { return onMutes(args...); });
    OscWiFi.subscribe(port, "/gains",
                      [=](auto &&...args) { return onGains(args...); });
    OscWiFi.subscribe(port, "/pans",
                      [=](auto &&...args) { return onPans(args...); });
    OscWiFi.subscribe(port, "/faders",
                      [=](auto &&...args) { return onFaders(args...); });
    OscWiFi.subscribe(port, "/mixes*",
                      [=](auto &&...args) { return onMixes(args...); });
    OscWiFi.subscribe(port, "/colors", [=](auto &&...args) {
      return onOscReceivedColors(args...);
    });

    // subscribe to /xremote messages
    OscWiFi.subscribe(port, "/headamp/*/gain",
                      [=](auto &&...args) { return onGain(args...); });
    OscWiFi.subscribe(port, "/ch/*/mix/pan",
                      [=](auto &&...args) { return onPan(args...); });
    OscWiFi.subscribe(port, "/ch/*/mix/fader",
                      [=](auto &&...args) { return onFade(args...); });
    OscWiFi.subscribe(port, "/ch/*/mix/*/level",
                      [=](auto &&...args) { return onMix(args...); });
    OscWiFi.subscribe(port, "/ch/*/mix/on",
                      [=](auto &&...args) { return onMute(args...); });
    OscWiFi.subscribe(port, "/rtn/aux/mix/fader",
                      [=](auto &&...args) { return onAux(args...); });
    OscWiFi.subscribe(port, "/lr/mix/fader",
                      [=](auto &&...args) { return onMain(args...); });

    // OscWiFi.subscribe(port, "/info", onOscReceivedSSSS);

    delay(100);

    // initialize values
    // receive mutes
    OscWiFi.send(host, port, "/formatsubscribe", "/mutes", "/ch/**/mix/on", 1,
                 16, 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    // receive faders
    OscWiFi.send(host, port, "/formatsubscribe", "/faders", "/ch/**/mix/fader",
                 1, 16, 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    // receive pans
    OscWiFi.send(host, port, "/formatsubscribe", "/pans", "/ch/**/mix/pan", 1,
                 16, 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    // receive gains
    OscWiFi.send(host, port, "/formatsubscribe", "/gains", "/headamp/**/gain",
                 1, 16, 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    // receive mixes
    String address = "/ch/**/mix/0X/level";
    String label = "/mixesX";
    for (uint8_t i = 1; i <= 6; i++) {
      label[6] = String(i)[0];
      address[12] = String(i)[0];
      OscWiFi.send(host, port, "/formatsubscribe", label, address, 1, 16, 80);
      OscWiFi.update();
      delay(DLY_SEND_INI);
    }

    // initially receive aux and main
    OscWiFi.send(host, port, "/subscribe", "/rtn/aux/mix/fader", 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);
    OscWiFi.send(host, port, "/subscribe", "/lr/mix/fader", 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    OscWiFi.publish(host, port, "/xremote")->setIntervalMsec(DLY_RENEW);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    // receive colors
    OscWiFi.send(host, port, "/formatsubscribe", "/colors",
                 "/ch/**/config/color", 1, 16, 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);

    OscWiFi.publish(host, port, "/renew", "/colors")
        ->setIntervalMsec(DLY_RENEW);
    OscWiFi.update();
    delay(DLY_SEND_INI);
    /*
      OscWiFi.send(k_host, port, "/info");
      delay(DLY_SEND);
      OscWiFi.send(k_host, port, "/ch/01/mix/on", 1);
      delay(DLY_SEND);
      OscWiFi.send(k_host, port, "/ch/01/config/color", 3);
      delay(DLY_SEND);
      */
  }

  void muteChannel(uint8_t id, bool val) {
    sprintf(buf, "/ch/%02d/mix/on", id + 1);
    OscWiFi.send(host, port, buf, (int)val);
  }
  void fadeMain(uint8_t val) {
    OscWiFi.send(host, port, "/lr/mix/fader", uInt2Float(val));
    sprintf(buf, "fadeMain(%d);", val);
    DEBUG_PRINTLN(buf);
  }
  void fadeAux(uint8_t val) {
    OscWiFi.send(host, port, "/rtn/aux/mix/fader", uInt2Float(val));
    sprintf(buf, "fadeAux(%d);", val);
    DEBUG_PRINTLN(buf);
  }
  void fadeChannel(uint8_t id, uint8_t val) {
    sprintf(buf, "/ch/%02d/mix/fader", id + 1);
    OscWiFi.send(host, port, buf, uInt2Float(val));
  }
  void panChannel(uint8_t id, uint8_t val) {
    sprintf(buf, "/ch/%02d/mix/pan", id + 1);
    OscWiFi.send(host, port, buf, uInt2Float(val));
  }
  void gainChannel(uint8_t id, uint8_t val) {
    sprintf(buf, "/headamp/%02d/gain", id + 1);
    OscWiFi.send(host, port, buf, uInt2Float(val));
  }
  void mixChannel(uint8_t id, uint8_t id_bus, uint8_t val) {
    sprintf(buf, "/ch/%02d/mix/%02d/level", id + 1, id_bus + 1);
    OscWiFi.send(host, port, buf, uInt2Float(val));
    // delay(DLY_SEND);
  }
  // osc subscription callbacks
  void onFade(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/ch/03/mix/fader) FLOAT(0.8162268)
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t fade = float2UInt(m.arg<float>(0));
    runAllCallbacks(onFadeCallbacks, id, fade);
  }
  void onPan(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/ch/04/mix/pan) FLOAT(0.51)
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t pan = float2UInt(m.arg<float>(0));
    runAllCallbacks(onPanCallbacks, id, pan);
  }
  void onGain(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/headamp/02/gain) FLOAT(0.5069444)
    printReceivedMessage(m);
    uint8_t id = parseHeadampChannel(m.address()) - 1;
    uint8_t gain = float2UInt(m.arg<float>(0));
    runAllCallbacks(onGainCallbacks, id, gain);
  }
  void onMix(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/ch/03/mix/04/level) FLOAT(0.1375)
    printReceivedMessage(m);
    uint8_t id_bus = parseMixBus(m.address()) - 1;
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t mix = float2UInt(m.arg<float>(0));
    runAllCallbacks(onMixCallbacks, id, mix, id_bus);
  }
  void onAux(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.254]:10024)
    // ADDRESS(/rtn/aux/mix/fader) FLOAT(0.007820137)
    printReceivedMessage(m);
    uint8_t aux = float2UInt(m.arg<float>(0));
    runAllCallbacks(onAuxCallbacks, aux);
  }
  void onMain(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.254]:10024)
    // ADDRESS(/lr/mix/fader) FLOAT(0.5434995)
    printReceivedMessage(m);
    uint8_t main = float2UInt(m.arg<float>(0));
    runAllCallbacks(onMainCallbacks, main);
  }

  void onMute(const OscMessage &m) {
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t mute = m.arg<int>(0);
    runAllCallbacks(onMuteCallbacks, id, mute == 0);
  }

  void onMutes(const OscMessage &m) {
    printReceivedMessage(m);
    int *temp_array =
        reinterpret_cast<int *>(&(m.arg<std::vector<char>>(0)[0]));
    for (uint8_t i = 1; i < m.arg<int32_t>(0) / sizeof(int); i++) {
      // array represents "on"
      runAllCallbacks(onMuteCallbacks, (uint8_t)(i - 1), temp_array[i] == 0);
      DEBUG_PRINT(temp_array[i] == 0);
      DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();
  }
  // initial float blob callbacks
  void onFloatBlob(const OscMessage &m, std::vector<cb_uu> v_cb) {
    printReceivedMessage(m);
    float *temp_array =
        reinterpret_cast<float *>(&(m.arg<std::vector<char>>(0)[0]));
    for (uint8_t i = 1; i < m.arg<int32_t>(0) / sizeof(float); i++) {
      runAllCallbacks(v_cb, (uint8_t)(i - 1), float2UInt(temp_array[i]));
    }
    DEBUG_PRINTLN();
  }
  void onFaders(const OscMessage &m) { return onFloatBlob(m, onFadeCallbacks); }
  void onPans(const OscMessage &m) { return onFloatBlob(m, onPanCallbacks); }
  void onGains(const OscMessage &m) { return onFloatBlob(m, onGainCallbacks); }
  void onMixes(const OscMessage &m) {
    uint8_t id_bus = parseMixesBus(m.address());
    printReceivedMessage(m);
    float *temp_array =
        reinterpret_cast<float *>(&(m.arg<std::vector<char>>(0)[0]));
    for (uint8_t i = 1; i < m.arg<int32_t>(0) / sizeof(float); i++) {
      runAllCallbacks(onMixCallbacks, (uint8_t)(i - 1),
                      float2UInt(temp_array[i]), id_bus);
    }
    DEBUG_PRINTLN();
  }

  void onOscReceivedColors(const OscMessage &m) {
    printReceivedMessage(m);
    int *color_array =
        reinterpret_cast<int *>(&(m.arg<std::vector<char>>(0)[0]));
    for (uint8_t i = 1; i < m.arg<int32_t>(0) / sizeof(uint32_t); i++) {
      runAllCallbacks(onColorCallbacks, (uint8_t)(i - 1),
                      (uint8_t)color_array[i]);
      DEBUG_PRINT(color_array[i]);
      DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();
  }
  // variable arguemtn style?
  template <typename... Args>
  void runAllCallbacks(std::vector<std::function<void(Args...)>> const &v_cb,
                       Args... args) {
    for (auto const &cb : v_cb) {
      cb(args...);
    }
  }
  // external reaction callbacks
  std::vector<cb_uu> onFadeCallbacks;
  void registerOnFadeCallback(const cb_uu &cb) {
    onFadeCallbacks.push_back(cb);
  }
  std::vector<cb_uu> onPanCallbacks;
  void registerOnPanCallback(const cb_uu &cb) { onPanCallbacks.push_back(cb); }
  std::vector<cb_uu> onGainCallbacks;
  void registerOnGainCallback(const cb_uu &cb) {
    onGainCallbacks.push_back(cb);
  }
  std::vector<cb_uuu> onMixCallbacks;
  void registerOnMixCallback(const cb_uuu &cb) { onMixCallbacks.push_back(cb); }
  std::vector<cb_u> onAuxCallbacks;
  void registerOnAuxCallback(const cb_u &cb) { onAuxCallbacks.push_back(cb); }
  std::vector<cb_u> onMainCallbacks;
  void registerOnMainCallback(const cb_u &cb) { onMainCallbacks.push_back(cb); }
  std::vector<cb_ub> onMuteCallbacks;
  void registerOnMuteCallback(const cb_ub &cb) {
    onMuteCallbacks.push_back(cb);
  }
  std::vector<cb_uu> onColorCallbacks;
  void registerOnColorCallback(const cb_uu &cb) {
    onColorCallbacks.push_back(cb);
  }
  void registerPrintCallbacks() {
    registerOnFadeCallback(onFadePrintln);
    registerOnPanCallback(onPanPrintln);
    registerOnGainCallback(onGainPrintln);
    registerOnMixCallback(onMixPrintln);
    registerOnAuxCallback(onAuxPrintln);
    registerOnMainCallback(onMainPrintln);
    registerOnMuteCallback(onMutePrintln);
    registerOnColorCallback(onColorPrintln);
  }

 private:
  char buf[30];
  char *host;
  uint16_t port;
  XTouchMiniMixer *xTouch;
  static uint8_t float2UInt(const float f) { return f * 127.0 + 0.5; };
  static float uInt2Float(const uint8_t u) { return ((float)u) / 127.0; };
  static int8_t delta_lin(int8_t delta_x, int8_t x0, int8_t y0) {
    // linearization from actual position to max/min
    if (delta_x > 0) {
      float m = (127.0 - y0) / (127.0 - x0);
      float n = y0 - m * x0;
      return m * delta_x + n;
    } else {
      float m = (float)y0 / (float)x0;
      return m * delta_x + 0;
    }
  }
  static uint8_t parseChannel(const String s) {
    return s.substring(4, 6).toInt();
  };
  static uint8_t parseHeadampChannel(const String s) {
    return s.substring(9, 11).toInt();
  };
  static uint8_t parseMixBus(const String s) {
    return s.substring(11, 13).toInt();
  };
  static uint8_t parseMixesBus(const String s) {
    return s.substring(6, 7).toInt();
  };
  static void printReceivedMessage(const OscMessage &m) {
    DEBUG_PRINT(m.remoteIP());
    DEBUG_PRINT(" ");
    DEBUG_PRINT(m.remotePort());
    DEBUG_PRINT(" ");
    DEBUG_PRINT(m.size());
    DEBUG_PRINT(" ");
    DEBUG_PRINT(m.address());
    DEBUG_PRINT(" ");
  }
  static void onFadePrintln(uint8_t id, uint8_t val) {
    DEBUG_PRINT("Channel ");
    DEBUG_PRINT(id + 1);
    DEBUG_PRINT(" fader set to ");
    DEBUG_PRINTLN(val);
  };
  static void onPanPrintln(uint8_t id, uint8_t val) {
    DEBUG_PRINT("Channel ");
    DEBUG_PRINT(id + 1);
    DEBUG_PRINT(" pan set to ");
    DEBUG_PRINTLN(val);
  };
  static void onGainPrintln(uint8_t id, uint8_t val) {
    DEBUG_PRINT("Channel ");
    DEBUG_PRINT(id + 1);
    DEBUG_PRINT(" gain set to ");
    DEBUG_PRINTLN(val);
  };
  static void onMixPrintln(uint8_t id, uint8_t val, uint8_t id_bus) {
    DEBUG_PRINT("Channel ");
    DEBUG_PRINT(id + 1);
    DEBUG_PRINT(" on bus ");
    DEBUG_PRINT(id_bus + 1);
    DEBUG_PRINT(" mix set to ");
    DEBUG_PRINTLN(val);
  };
  static void onMainPrintln(uint8_t val) {
    DEBUG_PRINT("Main set to ");
    DEBUG_PRINTLN(val);
  };
  static void onAuxPrintln(uint8_t val) {
    DEBUG_PRINT("Aux set to ");
    DEBUG_PRINTLN(val);
  };
  static void onMutePrintln(uint8_t id, bool val) {
    DEBUG_PRINT("Channel ");
    DEBUG_PRINT(id + 1);
    DEBUG_PRINT(" muted: ");
    DEBUG_PRINTLN(val);
  };
  static void onColorPrintln(uint8_t id, uint8_t val) {
    DEBUG_PRINT("Channel ");
    DEBUG_PRINT(id + 1);
    DEBUG_PRINT(" color: ");
    DEBUG_PRINTLN(val);
  }

 protected:
  // USBH_MIDI *pUSBH_MIDI;
  // USB *pUSB;
  // static void (*onInitUSBCallback)() = nullptr;
};