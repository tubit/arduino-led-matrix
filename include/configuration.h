#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION "0.0.1"

#include <Arduino.h>
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

#endif