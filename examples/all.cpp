/*
 *******************************************************************************
 * 16 Channel controller for Behringer xTouchMini MIDI USB controller.
 *******************************************************************************
 */
// XTouch in Standard Mode
// All buttons set to default (hold LayerA+B before connecting to power to
// reset)
// delay constants
// delay after sending OSC command can be reset to 0 with scheduled sending
#define DLY_SEND 0
// delay after sending OSC command in initialization routine 20ms
#define DLY_SEND_INI 50
// delay after which to renew OSC subscriptions. Needs to be smaller than
// 10000ms, I used factor 3 for safety, in case one or two messages slip
#define DLY_RENEW 3300.f
// delay after sending Midi command to XTouch device 5ms
//#define DLY_MIDI 10
// delay after sending Midi command to XTouch device 5ms
//#define DLY_COOL 200

#define XAIR_DUMMY
#define MY_DEBUG  // comment to not run debug

#ifndef MY_DEBUG
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#else
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#endif

#include <Arduino.h>

#include <xAirLED.hpp>
#include <xAirTM1638.hpp>
#include <xTouchMiniMixer.hpp>
#include <xAirController.hpp>

// delay constants
// delay after encoder is not touched anymore 1000ms
#define DLY_COOL 1000
// delay after sending Midi command to XTouch device 5ms
#define DLY_MIDI 5

USB Usb;
USBHub Hub(&Usb);
USBH_MIDI Midi(&Usb);
XTouchMiniMixer xTouch(&Midi, &Usb);

XAirTM1638 xTM;
XAirLED xLED = XAirLED();

XAirController xAir;

// WiFi credentials of XAir or router it is connected to
const char *k_ssid = "SSID";   
const char *k_pwd = "Password"; 

// ip address of mixer
// char *host_router = "192.168.88.254";
// char *host_ap = "192.168.1.1";
char *host = "192.168.0.125";
// UDP 10024 for xAir18
const uint16_t k_port = 10024;

bool wifi_down = 0;
bool wifi_back_up = 0;

#if defined(ESP8266)
// handler refrences for logging important WiFi events over serial
// WiFiEventHandler _onStationModeConnectedHandler;
WiFiEventHandler _onStationModeDisconnectedHandler;
WiFiEventHandler _onStationModeGotIPHandler;
// static functions for logging WiFi events to the UART
// static void onStationModeConnected(const WiFiEventStationModeConnected&
// event);
static void onStationModeDisconnected(
    const WiFiEventStationModeDisconnected &event) {
  // setupWifi();
  DEBUG_PRINTLN("Got disconnected!");
  // blinkAllButtons();
  wifi_down = 1;
}
void getHostIP() {
  /*if (WiFi.gatewayIP() == IPAddress(192, 168, 88, 1)) {
    host = host_router;
  } else {
    host = host_ap;
  }*/
}

static void onStationModeGotIP(const WiFiEventStationModeGotIP &event) {
  DEBUG_PRINT("WiFi connected, IP = ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINT("Gateway = ");
  DEBUG_PRINTLN(WiFi.gatewayIP());
  getHostIP();
  // host = host_router;
  wifi_back_up = 1;
  // setupOSC();
}

#elif defined(ESP_PLATFORM)
// static functions for logging WiFi events to the UART
// static void onStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info);
// static void onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t
// info); static void onStationModeGotIP(WiFiEvent_t event, WiFiEventInfo_t
// info);
#endif

void setupWifi() {
  // unfortunately xAir use insecure WEP method
  WiFi.enableInsecureWEP();
#ifdef ESP_PLATFORM
  WiFi.disconnect(true, true);  // disable wifi, erase ap info
  delay(1000);
  WiFi.mode(WIFI_STA);
#endif
  _onStationModeGotIPHandler = WiFi.onStationModeGotIP(onStationModeGotIP);
  WiFi.begin(k_ssid, k_pwd);
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT(".");
    delay(500);
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  _onStationModeDisconnectedHandler =
      WiFi.onStationModeDisconnected(onStationModeDisconnected);
#if defined(ESP8266)
  //_onStationModeConnectedHandler =
  // WiFi.onStationModeConnected(onStationModeConnected);
#elif defined(ESP_PLATFORM)
  // WiFi.onEvent(onStationModeConnected,
  // WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);
  // WiFi.onEvent(onStationModeDisconnected,
  // WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  // WiFi.onEvent(onStationModeGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
#endif
}

void setupX() {
  DEBUG_PRINTLN("ready");
  delay(100);
  xAir = XAirController(host, k_port);
  xAir.setup();
  xAir.registerPrintCallbacks();
}
void onInitUSB() {
  Serial.print("USB device connected: VID=");
  Serial.print(xTouch.getVID());
  Serial.print(", PID=");
  Serial.println(xTouch.getPID());
  xTouch.visualizeAll();
  xTouch.registerDebuggingCallbacks();
  xAir.registerXTouch(&xTouch);
}

// main
void setup() {
  Serial.begin(115200);
  delay(100);
  
  //xLED = XAirLED();
  setupWifi();
  xTouch.setup(onInitUSB);
  setupX();
  xAir.registerXLED(&xLED);
  xTM = XAirTM1638();
  xAir.registerXTM(&xTM);
}
void loop() {
  xTouch.update();
  xAir.update();
  xTM.update();
  xLED.update();
}
