#include <Arduino.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <SPI.h>
#include <FS.h>

#include "utf8ascii.h"

// We need to include a different set of libraries depending on our platform
#ifdef ARDUINO_ESP32_DEV
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #define WEBSERVER WebServer
  #include <SPIFFS.h>
#endif
#ifdef ARDUINO_ESP8266_WEMOS_D1MINILITE
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #define WEBSERVER ESP8266WebServer
#endif

// Name to announce via mDNS
#define MDNS_NAME "led1"

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

#define BUF_SIZE  75
char message[BUF_SIZE] = "Julian";
bool newMessageAvailable = true;

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

WiFiManager wifiManager;

WEBSERVER server(80);

void handleRoot() {
  if (server.method() == HTTP_POST)
  {
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

  while (*p != '\0')
  {
    charWidth = mx.getChar(utf8ascii(*p++), sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i <= charWidth; i++) // allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nConnecting");

  WiFi.setHostname(MDNS_NAME);
  wifiManager.autoConnect("LED Matrix");

  
  if (!MDNS.begin(MDNS_NAME))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("mDNS responder started");

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started\n");
 
  MDNS.addService("http", "tcp", 80);

  mx.begin();

  // config file test
  bool initok = false;
  SPI.begin();

  initok = SPIFFS.begin();
  if (!(initok))
  {
    Serial.println("Format SPIFFS");

  }
  if (initok) {
    Serial.println("SPIFFS is OK");
    File myfile = SPIFFS.open("/config", "r");
    String content = myfile.readStringUntil('\n');
    Serial.println(content);
  }

}

void loop() {
  //if (wifiMulti.run() == WL_CONNECTED) {
    server.handleClient();

    // print on MX
    if (newMessageAvailable)
    {
      Serial.println("Processing new message: " + String(message));
      scrollText(message);
      newMessageAvailable = false;
    }
  /*}
  else {
    Serial.println("WiFi not connected!");
    delay(1000);
  }*/
}