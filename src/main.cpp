#include "configuration.h"

#define CONFIG "/config.txt"
#define HOSTNAME "ESP-"

// Change the externalLight to the pin you wish to use if other than the Built-in LED
int notifyLight = LED_BUILTIN; // LED_BUILTIN is is the built in LED on the Wemos
int buttonWifiReset = 16;

// declare functions
void flashLED(int count, int delayTime);
void configModeCallback (WiFiManager *myWiFiManager);
int8_t getWifiQuality();

void handleForgetWifi();
void handleConfigure();
void sendHeader();
void sendFooter();

ESP8266WebServer server(WEBSERVER_PORT);
ESP8266HTTPUpdateServer serverUpdater;

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  //LittleFS.remove(CONFIG);
  delay(10);

  Serial.println();
  Serial.println("moin moin!");
  Serial.println("");

  // set output for external LED
  pinMode(notifyLight, OUTPUT);

  // set input for Button
  pinMode(buttonWifiReset, INPUT_PULLDOWN_16);

  // start WifiManager, set configCallbacks
  WiFiManager wifiManager;
  //wifiManager.resetSettings();

  // reset WiFi settings, blink forever
  if (digitalRead(buttonWifiReset)) {
    //TODO: scrollMessage("WiFi settings reset");
    Serial.println("WiFi reset button pressed while booting. Reset WiFi settings.");
    wifiManager.resetSettings();
    Serial.print("WiFi settings removed. Release button for reboot.");

    while( digitalRead(buttonWifiReset)) {
      flashLED(5, 100);
      Serial.print(".");
      flashLED(5, 50);
      delay(500);
    }

    ESP.reset();
    delay(5000);
  }

  wifiManager.setAPCallback(configModeCallback);

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {
    Serial.println("WiFi connection failed. Restarting.");
    delay(3000);
    WiFi.disconnect(true);
    ESP.reset();
    delay(5000);
  }

  // print the received signal strength:
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(getWifiQuality());
  Serial.println("%");

  if (WEBSERVER_ENABLED) {
    //server.on("/", displayStartPage);
    server.on("/forgetwifi", handleForgetWifi);
    server.on("/configure", handleConfigure);
    //server.onNotFound(redirectHome);
    serverUpdater.setup(&server, "/update", www_username, www_password);

    // Start the server
    server.begin();
    Serial.println("Webserver started");

    // Print the IP address
    String webAddress = "http://" + WiFi.localIP().toString() + ":" + String(WEBSERVER_PORT) + "/";
    Serial.println("Open URL: " + webAddress);
    //TODO: scrollMessage(" v" + String(VERSION) + "  IP: " + WiFi.localIP().toString() + "  ");
  } else {
    Serial.println("Web Interface is Disabled");
    //TODO: scrollMessage("Web Interface is Disabled");
  }

  flashLED(5, 500);
  Serial.println("Setup done. Starting loop.");
}

void loop() {
  delay(10);

  Serial.println(digitalRead(buttonWifiReset));

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }
//  if (ENABLE_OTA) {
//    ArduinoOTA.handle();
//  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println("Wifi Manager");
  Serial.println("Please connect to AP");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("To setup Wifi Configuration");
  //TODO: scrollMessage("Please Connect to AP: " + String(myWiFiManager->getConfigPortalSSID()));
  //TODO: centerPrint("wifi");
}

void flashLED(int count, int delayTime) {
  for (int inx = 0; inx < count; inx++) {
    digitalWrite(notifyLight, LOW);
    delay(delayTime);
    digitalWrite(notifyLight, HIGH);
    delay(delayTime);
  }
  digitalWrite(notifyLight, HIGH);
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

// Webserver
boolean authentication() {
  if (IS_BASIC_AUTH) {
    return server.authenticate(www_username, www_password);
  }
  return true;
}

void handleConfigure() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(notifyLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendHeader();

  sendFooter();

  server.sendContent("");
  server.client().stop();
}

// reset Wifi settings
void handleForgetWifi() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  //TODO: redirectHome();
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

void sendHeader() {
  String html = "<!DOCTYPE HTML>";
  html += "<html><head><title>Matrix Clock</title><link rel='icon' href='data:;base64,='>";
  html += "<meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
  html += "<link rel='stylesheet' href='https://www.w3schools.com/lib/w3-theme-" + themeColor + ".css'>";
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.8.1/css/all.min.css'>";
  html += "</head><body>";
  server.sendContent(html);
  html = "<nav class='w3-sidebar w3-bar-block w3-card' style='margin-top:88px' id='mySidebar'>";
  html += "<div class='w3-container w3-theme-d2'>";
  html += "<span onclick='closeSidebar()' class='w3-button w3-display-topright w3-large'><i class='fas fa-times'></i></span>";
  html += "<div class='w3-padding'>Menu</div></div>";
  server.sendContent(html);

  html = "</nav>";
  html += "<header class='w3-top w3-bar w3-theme'><button class='w3-bar-item w3-button w3-xxxlarge w3-hover-theme' onclick='openSidebar()'><i class='fas fa-bars'></i></button><h2 class='w3-bar-item'>Matrix Clock</h2></header>";
  html += "<script>";
  html += "function openSidebar(){document.getElementById('mySidebar').style.display='block'}function closeSidebar(){document.getElementById('mySidebar').style.display='none'}closeSidebar();";
  html += "</script>";
  html += "<br><div class='w3-container w3-large' style='margin-top:88px'>";
  server.sendContent(html);
}

void sendFooter() {
  int8_t rssi = getWifiQuality();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println("%");
  String html = "<br><br><br>";
  html += "</div>";
  html += "<footer class='w3-container w3-bottom w3-theme w3-margin-top'>";
  html += "<i class='far fa-paper-plane'></i> Version: " + String(VERSION) + "<br>";
  html += "<i class='fas fa-rss'></i> Signal Strength: ";
  html += String(rssi) + "%";
  html += "</footer>";
  html += "</body></html>";
  server.sendContent(html);
}
