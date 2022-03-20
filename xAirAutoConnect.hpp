#pragma once

// To properly include the suitable header files to the target platform.
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
using WiFiWebServer = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WebServer.h>
#include <WiFi.h>
using WiFiWebServer = WebServer;
#endif
#define AUTOCONNECT_URI "/_ac/config"
#define AUTOCONNECT_AP_GW IPAddress(192, 168, 1, 1)
#define AUTOCONNECT_AP_IP IPAddress(192, 168, 1, 1)
#include <AutoConnect.h>
#include <AutoConnectFS.h>

#include <xAirController.hpp>

XAirController* p_xAir = nullptr;
AutoConnectFS::FS& FlashFS = AUTOCONNECT_APPLIED_FILESYSTEM;
AutoConnectConfig config("xAirController", "");
AutoConnect portal;

String onRoot(AutoConnectAux& page, PageArgument& args) {
  if (p_xAir) {
    if (p_xAir->hosts.size()) {
      AutoConnectElement& devices =
          page.getElement<AutoConnectElement>("devices");
      devices.value = "";
      for (std::map<std::string, std::string>::iterator it =
               p_xAir->hosts.begin();
           it != p_xAir->hosts.end(); it++) {
        devices.value += "<button type='button' id='device-" +
                         String(std::distance(p_xAir->hosts.begin(), it)) +
                         "' onclick='connectTo(\"" + it->first.c_str() +
                         "\")'>" + it->second.c_str() + "<br>" +
                         it->first.c_str() + "</button>";
      }
    }
  }
  return String();
}
String onConnect(AutoConnectAux& page, PageArgument& args) {
  Serial.println(args.arg("host2"));
  if (args.arg("host")) {
    page["host_text"].value = args.arg("host");
    if (p_xAir) p_xAir->setHost(args.arg("host").c_str());
  } else {
    page["host_text"].value = "[none selected], aborting";
  }
  return String();
}

String onScan(AutoConnectAux& aux, PageArgument& args) {
  if (p_xAir) p_xAir->scanHosts();
  return String();
}

void xAuto_setup(XAirController* new_xAir = nullptr) {
  if (new_xAir) p_xAir = new_xAir;

  // enable OTA
  // config.ota = AC_OTA_BUILTIN;
  // remove HOME menu entry
  config.menuItems = config.menuItems & ~AC_MENUITEM_HOME;
  config.hostName = "xAirController";
  portal.config(config);

  FlashFS.begin(AUTOCONECT_FS_INITIALIZATION);

  File page = FlashFS.open("/custom_page.json", "r");
  portal.load(page);

  page.close();
  FlashFS.end();

  AutoConnectAux* aux = portal.aux("/");
  // AutoConnectSelect& select = aux->getElement<AutoConnectSelect>("host");
  // select.add("option 1");
  //  xAuto_addHost("other option");

  // callbacks
  portal.on("/", onRoot);
  portal.on("/connect", onConnect);
  portal.on("/scan", onScan);
  // unfortunately xAir use insecure WEP method
  WiFi.enableInsecureWEP();
  portal.begin();
}

void xAuto_update() { portal.handleClient(); }