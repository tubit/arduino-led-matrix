#include <Arduino.h>

// Use WiFiManager
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <SPI.h>
#include <FS.h>

// I need to translate UTF8 to ASCII for the LED Matrix
#include "utf8ascii.h"

// We need to include a different set of libraries depending on our platform
#ifdef ARDUINO_ESP32_DEV
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #define WEBSERVER WebServer
  #include <SPIFFS.h>
  #include <ArduinoOTA.h>
#endif
#ifdef ARDUINO_ESP8266_WEMOS_D1MINILITE
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #define WEBSERVER ESP8266WebServer
#endif

// Name to announce via mDNS
// TODO: Can we make this configurable via WiFiManager?
#define MDNS_NAME "led1"

/* LED Matrix controller & settings */
#include <MD_MAX72xx.h>
#define BUF_SIZE      75  // text buffer size
#define CHAR_SPACING  1   // pixels between characters

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

// depending on the target platform, I used different output ports
#ifdef ARDUINO_ESP32_DEV
  #define CLK_PIN   18  // CLK  or GPIO18
  #define DATA_PIN  23  // MOSI or GPIO23
  #define CS_PIN     5  // SS   or GPIO5
#endif
#ifdef ARDUINO_ESP8266_WEMOS_D1MINILITE
  #define CLK_PIN   14 // CLK  or IO14 or D5
  #define DATA_PIN  13 // MOSI or IO13 or D7
  #define CS_PIN    15 // SS   or IO15 or D8 
#endif

#define DELAYTIME 100 // in milliseconds
#define CHAR_SPACING  1 // pixels between characters

char message[BUF_SIZE] = "Hello World";
bool newMessageAvailable = true;

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

/* Let's go */
WiFiManager wifiManager;

WEBSERVER server(80);

void handleRoot() {
  if (server.method() == HTTP_POST) {
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i) == "message")
      {
        String m = server.arg(i);
        m += "            ";
        m.toCharArray(message, BUF_SIZE);
        newMessageAvailable = true;
      }
    }
    server.send(200, "text/plain", "Requests handled, thank you!");
  } else {
    server.send(200, "text/plan", "I can just work woth POST requests, sorry.");
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void scrollText(const char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts

  mx.clear();

  while (*p != '\0') {
    charWidth = mx.getChar(utf8ascii(*p++), sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i <= charWidth; i++) { // allow space between characters 
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }

    // handle http requests during scrolling
    // TODO: currently this overwrites the message.. We need to copy the text before scrolling and let it finish.
    server.handleClient();

  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting WiFiManager");

  //wifiManager.resetSettings();

  WiFi.setHostname(MDNS_NAME);
  bool wifi_connection = wifiManager.autoConnect();

 if (!wifi_connection) {
    Serial.println("WiFi connection failed");
    ESP.restart();
  } else {
    Serial.println("WiFi connection successful");
  }
  
  if (!MDNS.begin(MDNS_NAME)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }

#ifndef ARDUINO_ESP8266_WEMOS_D1MINILITE
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
#endif

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started\n");
 
  MDNS.addService("http", "tcp", 80);

  mx.begin();
  String hn = WiFi.getHostname();
  hn += "            ";
  hn.toCharArray(message, BUF_SIZE);
  newMessageAvailable = true;
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
#ifndef ARDUINO_ESP8266_WEMOS_D1MINILITE
    ArduinoOTA.handle();
#endif
    server.handleClient();

    if (newMessageAvailable) {
      Serial.println("Processing message: " + String(message));
      scrollText(message);
      //newMessageAvailable = false;
    }
  }
  else {
    Serial.println("WiFi not connected!");
    delay(1000);
  }
}