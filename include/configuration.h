#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>

/* all for wifi and webserver */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h> // --> https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <FS.h>

#define WEBSERVER ESP8266WebServer

// Settings for WiFiManager
const int WEBSERVER_PORT = 80; // The port you can access this device on over HTTP
const boolean WEBSERVER_ENABLED = true;  // Device will provide a web interface via http://[ip]:[port]/
boolean IS_BASIC_AUTH = false;  // Use Basic Authorization for Configuration security on Web Interface
char* www_username = "admin";  // User account for the Web Interface
char* www_password = "password";  // Password for the Web Interface

String themeColor = "blue-grey"; // this can be changed later in the web interface.

// Output PINs for the MAX72xx matrix display
#define CLK_PIN   14 // CLK  or IO14 or D5
#define DATA_PIN  13 // MOSI or IO13 or D7
#define CS_PIN    15 // SS   or IO15 or D8 

#define DELAYTIME 100   // in milliseconds
#define CHAR_SPACING  1 // pixels between characters

/* Time settings */
#include <Time.h>
const long gmt_offset_sec = 3600;
const int  day_light_offset_sec = 3600;

boolean ENABLE_OTA = true;    // this will allow you to load firmware to the device over WiFi (see OTA for ESP8266)
String OTA_Password = "";     // Set an OTA password here -- leave blank if you don't want to be prompted for password

boolean DISPLAY_SECOND_TICK = false; // decide if we want to have a visible second "tick tack" in the top right corner
int INTENSITY_CLOCK     = 0;
int INTENSITY_TEXT      = 2;
int INTENSITY_ANIMATION = 2;

int notifyLight = LED_BUILTIN; // LED_BUILTIN is is the built in LED on the Wemos

#endif