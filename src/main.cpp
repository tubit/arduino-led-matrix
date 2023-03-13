#include <Arduino.h>
#include <WiFi.h>
/*
#include <WiFiMulti.h>
*/
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <MD_MAX72xx.h>

#include <WebServer.h>
#include <ESPmDNS.h>

#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>

#include "utf8ascii.h"

// Name to announce via mDNS
#define MDNS_NAME "led1"

#define BUF_SIZE      75  // text buffer size
#define CHAR_SPACING  1   // pixels between characters

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   18  // or SCK
#define DATA_PIN  23  // or MOSI
#define CS_PIN     5  // or 

#define DELAYTIME 100 // in milliseconds
#define CHAR_SPACING  1 // pixels between characters

// Global message buffers shared by Serial and Scrolling functions
#define BUF_SIZE  75
char message[BUF_SIZE] = "Julian";
bool newMessageAvailable = true;

// how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 1

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

/*
WiFiMulti wifiMulti;

const char* ssid = "konert.me";
const char* password = "md020413jk14";
*/
WiFiManager wifiManager;

WebServer server(80);

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

/*
  wifiMulti.addAP(ssid, password);

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println();
      Serial.println("WiFi connected");
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
      break;
    }
    else {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }
*/
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