#pragma once

#define ARDUINOOSC_DEBUGLOG_ENABLE
#define ARDUINOOSC_ENABLE_WIFI
#define ARDUINOOSC_ENABLE_BUNDLE
#define ARDUINOOSC_MAX_MSG_BUNDLE_SIZE 138

// Please include ArduinoOSCWiFi.h to use ArduinoOSC on the platform
// which can use both WiFi and Ethernet
#include <ArduinoOSCWiFi.h>

#include <xAirLED.hpp>
#include <xAirTM1638.hpp>
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
typedef std::function<void(uint8_t, char *)> cb_upc;

class XAirController {
 public:
  int i;
  bool b_init = false;
  XAirController(){
      // osc_buf = (char*)malloc(30);
  };
  XAirController(uint16_t new_port) {
    // osc_buf = (char*)malloc(30);
    // find host
    port = new_port;
  };
  std::map<std::string, std::string> hosts;

  void findHosts() {
    char ip[13];
    char host_temp[16];
    sprintf(ip, "%d.%d.%d.", WiFi.gatewayIP()[0], WiFi.gatewayIP()[1],
            WiFi.gatewayIP()[2]);
    for (uint8_t i = 1; i != 0; i++) {
      if (i != WiFi.gatewayIP()[3] && i != WiFi.localIP()[3]) {
        sprintf(host_temp, "%s%d", ip, i);
        OscWiFi.send(host_temp, port, "/status", 80);
        OscWiFi.update();
      }
    }
  }

  void registerHost(const OscMessage &m) {
    DEBUG_PRINT("register Host ");
    DEBUG_PRINT(m.arg<String>(1).c_str());
    DEBUG_PRINT(" ");
    DEBUG_PRINTLN(m.arg<String>(2).c_str());
    if (host[0] == '\0') {
      // no host was entered before (e.g. from EEPROM or passed via initializer)
      DEBUG_PRINT("host was emtpy, now setup with: ");
      strcpy(host, m.arg<String>(1).c_str());
      DEBUG_PRINTLN(host);
      b_init = true;
    }
    hosts[m.arg<String>(1).c_str()] = m.arg<String>(2).c_str();
  };

  XAirController(uint16_t new_port, char *new_host) {
    // osc_buf = (char*)malloc(30);
    strcpy(host, new_host);
    port = new_port;
  };
  XAirController(uint16_t new_port, char *new_host,
                 XTouchMiniMixer *new_xTouch) {
    // osc_buf = (char*)malloc(30);
    strcpy(host, new_host);
    port = new_port;
    registerPrintCallbacks();
    registerXTouch(new_xTouch);
  };
  void update() {
    if (b_init) {
      init();
      b_init = false;
    }
    OscWiFi.update();
  }
  void registerXLED(XAirLED *new_xLED) {
    xLED = new_xLED;
    // register xTM callbacks in xAir
    registerOnMuteCallback(
        [=](uint8_t id, bool val) { return xLED->setMuted(id, val); });
    registerOnColorCallback(
        [=](auto &&...args) { return xLED->setColor(args...); });
  }
  void registerXTM(XAirTM1638 *new_xTM) {
    xTM = new_xTM;
    // register xTM callbacks in xAir
    registerOnMuteCallback(
        [=](uint8_t id, bool val) { return xTM->setMuted(id, val); });
    // register xTM callbacks in xAir
    registerOnNameCallback(
        [=](uint8_t id, char *name) { return xTM->setName(id, name); });

    // register xAir callbacks in xTM
    xTM->registerMuteChannelCallback(
        [=](auto &&...args) { return muteChannel(args...); });
    xTM->registerGetNameCallback(
        [=](auto &&...args) { return getName(args...); });
  }
  void registerXTouch(XTouchMiniMixer *new_xTouch) {
    xTouch = new_xTouch;
    // register xTouch callbacks in xAir
    registerOnMuteCallback(
        [=](auto &&...args) { return xTouch->setMuted(args...); });
    registerOnFadeCallback(
        [=](auto &&...args) { return xTouch->setFaded(args...); });
    registerOnPanCallback(
        [=](auto &&...args) { return xTouch->setPanned(args...); });
    registerOnGainCallback(
        [=](auto &&...args) { return xTouch->setGained(args...); });
    registerOnMixCallback(
        [=](auto &&...args) { return xTouch->setMixed(args...); });
    registerOnMainCallback(
        [=](auto &&...args) { return xTouch->setMainFaded(args...); });
    registerOnAuxCallback(
        [=](auto &&...args) { return xTouch->setAuxFaded(args...); });
    registerOnColorCallback(
        [=](auto &&...args) { return xTouch->setColored(args...); });

    // register xAir callbacks in xTouch
    xTouch->registerMuteChannelCallback(
        [=](auto &&...args) { return muteChannel(args...); });
    xTouch->registerFadeChannelCallback(
        [=](auto &&...args) { return fadeChannel(args...); });
    xTouch->registerPanChannelCallback(
        [=](auto &&...args) { return panChannel(args...); });
    xTouch->registerGainChannelCallback(
        [=](auto &&...args) { return gainChannel(args...); });
    xTouch->registerMixChannelCallback(
        [=](auto &&...args) { return mixChannel(args...); });
    xTouch->registerFadeMainCallback(
        [=](auto &&...args) { return fadeMain(args...); });
    xTouch->registerFadeAuxCallback(
        [=](auto &&...args) { return fadeAux(args...); });
    /*bind alternative with explicit arguments
    xTouch->registerMuteChannelCallback(std::bind(&XAirController::muteChannel,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2));*/
  }
  void runAllMuteCallbacks(auto &&...args) {
    evokeAllCallbacks(&onMuteCallbacks, args...);
  }
  void subscribe() {
    // Behringer OSC requires both server and client ports to be the same
    // subscribe to init messages
    OscWiFi.subscribe(port, "/mutes",
                      [&](const OscMessage &m) { return onMutes(m); });
    OscWiFi.subscribe(port, "/faders",
                      [&](const OscMessage &m) { return onFaders(m); });
    OscWiFi.subscribe(port, "/pans",
                      [&](const OscMessage &m) { return onPans(m); });
    OscWiFi.subscribe(port, "/gains",
                      [&](const OscMessage &m) { return onGains(m); });
    OscWiFi.subscribe(port, "/mixes*",
                      [&](const OscMessage &m) { return onMixes(m); });
    OscWiFi.subscribe(port, "/colors",
                      [&](const OscMessage &m) { return onColors(m); });
    // subscribe to messages on demand (e.g. name)
    OscWiFi.subscribe(port, "/ch/*/config/name",
                      [&](const OscMessage &m) { return onName(m); });
    // subscribe to /xremote messages
    OscWiFi.subscribe(port, "/headamp/*/gain",
                      [&](const OscMessage &m) { return onGain(m); });
    OscWiFi.subscribe(port, "/ch/*/mix/pan",
                      [&](const OscMessage &m) { return onPan(m); });
    OscWiFi.subscribe(port, "/ch/*/mix/fader",
                      [&](const OscMessage &m) { return onFade(m); });
    OscWiFi.subscribe(port, "/ch/*/mix/*/level",
                      [&](const OscMessage &m) { return onMix(m); });
    OscWiFi.subscribe(port, "/ch/*/mix/on",
                      [&](const OscMessage &m) { return onMute(m); });
    OscWiFi.subscribe(port, "/lr/mix/fader",
                      [&](const OscMessage &m) { return onMain(m); });
    OscWiFi.subscribe(port, "/rtn/aux/mix/fader",
                      [&](const OscMessage &m) { return onAux(m); });
    // OscWiFi.subscribe(port, "/info", onOscReceivedSSSS);
    OscWiFi.subscribe(port, "/status",
                      [&](const OscMessage &m) { return registerHost(m); });
  };
  void setup() {
    subscribe();
    if (host[0] == 0) {
      findHosts();
    } else {
      init();
    }
  };
  void init() {
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

    // initially receive main and aux
    OscWiFi.send(host, port, "/subscribe", "/lr/mix/fader", 80);
    OscWiFi.update();
    delay(DLY_SEND_INI);
    OscWiFi.send(host, port, "/subscribe", "/rtn/aux/mix/fader", 80);
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
    // OscWiFi.send(host, port, "/info");
    // delay(DLY_SEND_INI);
  }

  void muteChannel(uint8_t id, bool val) {
    sprintf(osc_buf, "/ch/%02d/mix/on", id + 1);
    OscWiFi.send(host, port, osc_buf, (int)val);
  }
  void fadeMain(uint8_t val) {
    OscWiFi.send(host, port, "/lr/mix/fader", uInt2Float(val));
  }
  void fadeAux(uint8_t val) {
    OscWiFi.send(host, port, "/rtn/aux/mix/fader", uInt2Float(val));
  }
  void fadeChannel(uint8_t id, uint8_t val) {
    // char osc_buf[25];
    sprintf((char *)osc_buf, "/ch/%02d/mix/fader", id + 1);
    // osc_buf[25] = '\0';
    OscWiFi.send(host, port, (char *)osc_buf, uInt2Float(val));
  }
  void panChannel(uint8_t id, uint8_t val) {
    sprintf(osc_buf, "/ch/%02d/mix/pan", id + 1);
    OscWiFi.send(host, port, osc_buf, uInt2Float(val));
  }
  void gainChannel(uint8_t id, uint8_t val) {
    sprintf(osc_buf, "/headamp/%02d/gain", id + 1);
    OscWiFi.send(host, port, osc_buf, uInt2Float(val));
  }
  void mixChannel(uint8_t id, uint8_t id_bus, uint8_t val) {
    sprintf(osc_buf, "/ch/%02d/mix/%02d/level", id + 1, id_bus + 1);
    OscWiFi.send(host, port, osc_buf, uInt2Float(val));
  }
  void getName(uint8_t id) {
    sprintf(osc_buf, "/ch/%02d/config/name", id + 1);
    OscWiFi.send(host, port, "/subscribe", (String)osc_buf, 80);
  }
  // osc subscription callbacks
  void onFade(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/ch/03/mix/fader) FLOAT(0.8162268)
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t fade = float2UInt(m.arg<float>(0));
    evokeAllCallbacks(&onFadeCallbacks, id, fade);
  }
  void onPan(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/ch/04/mix/pan) FLOAT(0.51)
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t pan = float2UInt(m.arg<float>(0));
    evokeAllCallbacks(&onPanCallbacks, id, pan);
  }
  void onGain(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/headamp/02/gain) FLOAT(0.5069444)
    printReceivedMessage(m);
    uint8_t id = parseHeadampChannel(m.address()) - 1;
    uint8_t gain = float2UInt(m.arg<float>(0));
    evokeAllCallbacks(&onGainCallbacks, id, gain);
  }
  void onMix(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.252]:10024)
    // ADDRESS(/ch/03/mix/04/level) FLOAT(0.1375)
    printReceivedMessage(m);
    uint8_t id_bus = parseMixBus(m.address()) - 1;
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t mix = float2UInt(m.arg<float>(0));
    evokeAllCallbacks(&onMixCallbacks, id, id_bus, mix);
  }
  void onAux(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.254]:10024)
    // ADDRESS(/rtn/aux/mix/fader) FLOAT(0.007820137)
    printReceivedMessage(m);
    uint8_t aux = float2UInt(m.arg<float>(0));
    evokeAllCallbacks(&onAuxCallbacks, aux);
  }
  void onMain(const OscMessage &m) {
    // RECEIVE    | ENDPOINT([::ffff:192.168.88.254]:10024)
    // ADDRESS(/lr/mix/fader) FLOAT(0.5434995)
    printReceivedMessage(m);
    uint8_t main = float2UInt(m.arg<float>(0));
    evokeAllCallbacks(&onMainCallbacks, main);
  }

  void onMute(const OscMessage &m) {
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    uint8_t mute = m.arg<int>(0);
    evokeAllCallbacks(&onMuteCallbacks, id, mute == 0);
  }

  void onMutes(const OscMessage &m) {
    printReceivedMessage(m);
    int *temp_array =
        reinterpret_cast<int *>(&(m.arg<std::vector<char>>(0)[0]));
    for (uint8_t i = 1; i < m.arg<int32_t>(0) / sizeof(int); i++) {
      // array represents "on"
      evokeAllCallbacks(&onMuteCallbacks, (uint8_t)(i - 1), temp_array[i] == 0);
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
      evokeAllCallbacks(&v_cb, (uint8_t)(i - 1), float2UInt(temp_array[i]));
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
      evokeAllCallbacks(&onMixCallbacks, (uint8_t)(i - 1), id_bus,
                        float2UInt(temp_array[i]));
    }
    DEBUG_PRINTLN();
  }

  void onColors(const OscMessage &m) {
    printReceivedMessage(m);
    int *color_array =
        reinterpret_cast<int *>(&(m.arg<std::vector<char>>(0)[0]));
    for (uint8_t i = 1; i < m.arg<int32_t>(0) / sizeof(uint32_t); i++) {
      evokeAllCallbacks(&onColorCallbacks, (uint8_t)(i - 1),
                        (uint8_t)color_array[i]);
      DEBUG_PRINT(color_array[i]);
      DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();
  }
  void onName(const OscMessage &m) {
    printReceivedMessage(m);
    uint8_t id = parseChannel(m.address()) - 1;
    char name[12];
    strcpy(name, m.arg<String>(0).c_str());
    evokeAllCallbacks(&onNameCallbacks, id, name);
  }
  // variable argument style
  template <typename... Args>
  void evokeAllCallbacks(std::vector<std::function<void(Args...)>> *v_cb,
                         Args... args) {
    DEBUG_PRINT("evokeAllCallbacks, the are ");
    DEBUG_PRINTLN(v_cb->size());
    for (auto cb : *v_cb) {
      cb(args...);
    }
  }
  // external reaction callbacks
  std::vector<cb_uu> onFadeCallbacks;
  void registerOnFadeCallback(const cb_uu cb) { onFadeCallbacks.push_back(cb); }
  std::vector<cb_uu> onPanCallbacks;
  void registerOnPanCallback(const cb_uu cb) { onPanCallbacks.push_back(cb); }
  std::vector<cb_uu> onGainCallbacks;
  void registerOnGainCallback(const cb_uu cb) { onGainCallbacks.push_back(cb); }
  std::vector<cb_uuu> onMixCallbacks;
  void registerOnMixCallback(const cb_uuu cb) { onMixCallbacks.push_back(cb); }
  std::vector<cb_u> onAuxCallbacks;
  void registerOnAuxCallback(const cb_u cb) { onAuxCallbacks.push_back(cb); }
  std::vector<cb_u> onMainCallbacks;
  void registerOnMainCallback(const cb_u cb) { onMainCallbacks.push_back(cb); }
  std::vector<cb_ub> onMuteCallbacks;
  void registerOnMuteCallback(const cb_ub cb) { onMuteCallbacks.push_back(cb); }
  std::vector<cb_uu> onColorCallbacks;
  void registerOnColorCallback(const cb_uu cb) {
    onColorCallbacks.push_back(cb);
  }
  std::vector<cb_upc> onNameCallbacks;
  void registerOnNameCallback(const cb_upc cb) {
    onNameCallbacks.push_back(cb);
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
  char osc_buf[30];
  char host[16] = "";
  uint16_t port;
  XTouchMiniMixer *xTouch;
  XAirTM1638 *xTM;
  XAirLED *xLED;
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
    DEBUG_PRINTLN(" ");
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
  static void onMixPrintln(uint8_t id, uint8_t id_bus, uint8_t val) {
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
};