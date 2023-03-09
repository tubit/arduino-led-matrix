#include <Arduino.h>

// Use WiFiManager
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <SPI.h>
#include <FS.h>
#include <Time.h>

// I need to translate UTF8 to ASCII for the LED Matrix
#include "utf8ascii.h"
#include "effects.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define WEBSERVER ESP8266WebServer

/* LED Matrix controller & settings */
#include <MD_MAX72xx.h>
#define BUF_SIZE      75  // text buffer size
#define CHAR_SPACING  1   // pixels between characters

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

// Output PINs for the MAX72xx matrix display
#define CLK_PIN   14 // CLK  or IO14 or D5
#define DATA_PIN  13 // MOSI or IO13 or D7
#define CS_PIN    15 // SS   or IO15 or D8 

#define DELAYTIME 100   // in milliseconds
#define CHAR_SPACING  1 // pixels between characters

// for message scrolling
char message[BUF_SIZE] = "Hello World";
bool newMessageAvailable = true;
bool loop_message = true;

int animation = 0;

// for clock
const long gmt_offset_sec = 3600;
const int  day_light_offset_sec = 3600;
bool update_matrix = false;
uint8_t time_hour = 255;
uint8_t time_minute = 255;

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

/* Let's go */
WiFiManager wifiManager;

WEBSERVER server(80);

// poor mans http api
void handleRoot() {
  if (server.method() == HTTP_POST) {
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i) == "message") {
        String m = server.arg(i);
        m += "            ";
        m.toCharArray(message, BUF_SIZE);
        newMessageAvailable = true;
      } else if (server.argName(i) == "animation") {
        animation = server.arg(i).toInt();
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

void scrollText(const char *p) {
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

// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
void printText(uint8_t modStart, uint8_t modEnd, char *pMsg) {
  uint8_t   state = 0;
  uint8_t   curLen = 0;
  uint16_t  showLen = 0;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do     // finite state machine to print the characters in the space available
  {
    switch(state) {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0') {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // no break !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen) {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3:	// display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting WiFiManager");

  //wifiManager.resetSettings();

  bool wifi_connection = wifiManager.autoConnect();

 if (!wifi_connection) {
    Serial.println("WiFi connection failed");
    ESP.restart();
  } else {
    Serial.println("WiFi connection successful");
  }

 /* 
  if (!MDNS.begin(MDNS_NAME)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }
*/

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

  // set timezone
  configTime(gmt_offset_sec, day_light_offset_sec, "0.de.pool.ntp.org", "1.de.pool.ntp.org", "2.de.pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Timezone Europe/Berlin
  tzset();

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started\n");
 
  //MDNS.addService("http", "tcp", 80);

  mx.begin();
  String hn = WiFi.getHostname();
  hn += "            ";
  hn.toCharArray(message, BUF_SIZE);
  newMessageAvailable = true;
}

void loop() {
  struct tm timedate;

  if (WiFi.status() == WL_CONNECTED) {
#ifndef ARDUINO_ESP8266_WEMOS_D1MINILITE
    ArduinoOTA.handle();
#endif
    server.handleClient();

    time_t now = time(&now);
    localtime_r(&now, &timedate);

    if (newMessageAvailable) {
      Serial.println("Processing message: " + String(message));
      mx.control(MD_MAX72XX::INTENSITY, 2);
      scrollText(message);
      newMessageAvailable = false;
      update_matrix = true;
    }

    if (animation > 0) {
      mx.control(MD_MAX72XX::INTENSITY, 2);
      switch (animation) {
        case 1: bullseye(&mx); break;
        case 2: bounce(&mx); break;
        case 3: cross(&mx); break;
        case 4: stripe(&mx); break;
        case 5: spiral(&mx); break;
        case 6: rows(&mx); break;
        case 7: columns(&mx); break;
        case 8: checkboard(&mx); break;
        case 9: transformation1(&mx); break;
      }

      animation = 0;
      update_matrix = true;
    }

    if (timedate.tm_min != time_minute || timedate.tm_hour != time_hour || update_matrix == true) {
      update_matrix = false;

      time_minute = timedate.tm_min;
      time_hour = timedate.tm_hour;

      Serial.println("Update clock");
      Serial.print("Hour: ");
      Serial.println(time_hour);
      Serial.print("Minute: ");
      Serial.println(time_minute);

      String current_time = " ";
      if (time_hour < 10) current_time += "0";
      current_time += (String)time_hour + ":";
      if (time_minute < 10) current_time += "0";
      current_time += (String)time_minute;

      current_time.toCharArray(message, BUF_SIZE);
      mx.control(MD_MAX72XX::INTENSITY, 0);
      printText(0, MAX_DEVICES-1, message);
    }

    delay(1);
  } else {
    Serial.println("WiFi not connected!");
    delay(1000);
  }
}