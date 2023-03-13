#include "configuration.h"
#include "utf8ascii.h"
#include "effects.h"

#define VERSION "0.0.1"
#define CONFIG "/config.txt"
#define HOSTNAME "ESP-"

// Change the externalLight to the pin you wish to use if other than the Built-in LED
int notifyLight = LED_BUILTIN; // LED_BUILTIN is is the built in LED on the Wemos
int buttonWifiReset = 16;

/* declare functions */
void flashLED(int count, int delayTime);
void configModeCallback (WiFiManager *myWiFiManager);
int8_t getWifiQuality();

void handleRoot();
void handleForgetWifi();
void handleConfigure();
void sendHeader();
void sendFooter();

void printText(uint8_t modStart, uint8_t modEnd, char *pMsg);
void scrollText(const char *p);
/* end declare functions */

ESP8266WebServer server(WEBSERVER_PORT);
ESP8266HTTPUpdateServer serverUpdater;
int8_t hold_seconds = 0;

// for message scrolling
char message[BUF_SIZE] = "Hello World";
bool newMessageAvailable = true;
bool loop_message = true;

int animation = 0;

bool update_matrix = false;

// clock data
uint8_t time_hour = 255;
uint8_t time_minute = 255;

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

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

  mx.begin();
  Serial.println("Matrix created");
  mx.control(MD_MAX72XX::INTENSITY, 0);
  String moin = "moin!";
  moin.toCharArray(message, BUF_SIZE);
  printText(0, MAX_DEVICES-1, message);

  for (int inx = 0; inx <= 15; inx++) {
    mx.control(MD_MAX72XX::INTENSITY, inx);
    delay(100);
  }
  
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

  // dim down intensity again
  for (int inx = 15; inx >= 0; inx--) {
    mx.control(MD_MAX72XX::INTENSITY, inx);
    delay(60);
  }

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
    server.on("/", handleRoot);
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

  // Scroll your own hostname after first boot.
  String hn = WiFi.getHostname();
  hn += "            ";
  hn.toCharArray(message, BUF_SIZE);
  newMessageAvailable = true;

  // set timezone
  configTime(gmt_offset_sec, day_light_offset_sec, "0.de.pool.ntp.org", "1.de.pool.ntp.org", "2.de.pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Timezone Europe/Berlin
  tzset();

  Serial.println("Setup done. Starting loop.");
}

void loop() {
  struct tm timedate;

  time_t now = time(&now);
  localtime_r(&now, &timedate);

  if (digitalRead(buttonWifiReset)) {
    hold_seconds++;
    Serial.print(digitalRead(buttonWifiReset)); Serial.print(" - ");
    Serial.println(hold_seconds);
  } else {
    hold_seconds = 0;
  }

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }
//  if (ENABLE_OTA) {
//    ArduinoOTA.handle();
//  }

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

    String current_time = " ";
    if (time_hour < 10) current_time += "0";
    current_time += (String)time_hour + ":";
    if (time_minute < 10) current_time += "0";
    current_time += (String)time_minute;

    Serial.print("Update clock: ");
    Serial.println(current_time);

    current_time.toCharArray(message, BUF_SIZE);
    mx.control(MD_MAX72XX::INTENSITY, 0);
    printText(0, MAX_DEVICES-1, message);
  }
  delay(10);
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
      if (WEBSERVER_ENABLED) {
        server.handleClient();
      }
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
