#include "configuration.h"
#include <U8g2lib.h>

#define VERSION "1.0.0"
#define CONFIG "/config.txt"
#define HOSTNAME "ESP-"

// adapt and use: https://github.com/upiir/arduino_matrix_display_max7219_u8g2/blob/main/arduino_matrix_yt_counter.ino

void configModeCallback (WiFiManager *myWiFiManager);
int8_t getWifiQuality();

void redirectHome();
void handleRoot();
void handleForgetWifi();
void handleForgetConfig();
void handleConfigure();
void sendHeader();
void sendFooter();
void handleSaveConfig();

void readConfig();
void writeConfig();

void resetWifiConfig();

/* end declare functions */

ESP8266WebServer server(WEBSERVER_PORT);
ESP8266HTTPUpdateServer serverUpdater;
int16_t hold_seconds = 0;

bool update_matrix = false;

// clock data
uint8_t time_hour = 255;
uint8_t time_minute = 255;
uint8_t time_second = 255;

U8G2_MAX7219_32X8_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ CLK_PIN, /* data=*/ DATA_PIN, /* cs=*/ CS_PIN, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);

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

  u8g2.begin(); // begin function is required for u8g2 library
  u8g2.setContrast(0); // set display contrast 0-255

  u8g2.clearBuffer();	// clear the internal u8g2 memory
  u8g2.setFont(u8g2_font_minuteconsole_tr);	// choose a suitable font with digits 3px wide
 
  u8g2.drawStr(16, 8, "moin");
  u8g2.sendBuffer();

  for (int i = 0; i<255; i++) { u8g2.setContrast(i); delay(5); }
  delay(10);
  for (int i = 255; i>0; i--) { u8g2.setContrast(i); delay(5); }

  // start WifiManager, set configCallbacks
  WiFiManager wifiManager;
  //wifiManager.resetSettings();

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

  if (ENABLE_OTA) {
    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname((const char *)hostname.c_str());
    if (OTA_Password != "") {
      ArduinoOTA.setPassword(((const char *)OTA_Password.c_str()));
    }
    ArduinoOTA.begin();
  }

  if (WEBSERVER_ENABLED) {
    server.on("/", handleRoot);
    server.on("/forgetwifi", handleForgetWifi);
    server.on("/forgetconfig", handleForgetConfig);
    server.on("/configure", handleConfigure);
    server.on("/saveconfig", handleSaveConfig);
    server.onNotFound(redirectHome);
    serverUpdater.setup(&server, "/update");
    //serverUpdater.setup(&server, "/update", www_username, www_password);

    // Start the server
    server.begin();
    Serial.println("Webserver started");

    // Print the IP address
    String webAddress = "http://" + WiFi.localIP().toString() + ":" + String(WEBSERVER_PORT) + "/";
    Serial.println("Open URL: " + webAddress);
  } else {
    Serial.println("Web Interface is Disabled");
  }

  // set timezone
  configTime(gmt_offset_sec, day_light_offset_sec, "0.de.pool.ntp.org", "1.de.pool.ntp.org", "2.de.pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Timezone Europe/Berlin
  tzset();

  readConfig();

  Serial.println("Setup done. Starting loop.");
}

void loop() {
  struct tm timedate;
  static bool ticktack = true;

  time_t now = time(&now);
  localtime_r(&now, &timedate);

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }
  if (ENABLE_OTA) {
    ArduinoOTA.handle();
  }

  if (timedate.tm_min != time_minute || timedate.tm_hour != time_hour || update_matrix == true || (DISPLAY_SECOND_TICK && time_second != timedate.tm_sec)) {
    update_matrix = false;

    time_minute = timedate.tm_min;
    time_hour = timedate.tm_hour;

    Serial.printf("Update clock: %02d:%02d\n", time_hour, time_minute); 

    u8g2.clearBuffer();	// clear the internal u8g2 memory
    u8g2.setFont(u8g2_font_minuteconsole_tr);	// choose a suitable font with digits 3px wide
    u8g2.setContrast(INTENSITY_CLOCK * 10);
    
    char digit_char[2];
    itoa(time_hour, digit_char, 10);
    if (time_hour < 10) { digit_char[1] = digit_char[0]; digit_char[0] = '0'; }
    u8g2.drawStr(16, 8, digit_char); // hour

    itoa(time_minute, digit_char, 10);
    if (time_minute < 10) { digit_char[1] = digit_char[0]; digit_char[0] = '0'; }
    u8g2.drawStr(25, 8, digit_char); // minute

    if (DISPLAY_SECOND_TICK) {
      u8g2.setDrawColor(((ticktack) ? 0 : 1));
      u8g2.drawPixel(23, 7);
      u8g2.setDrawColor(1);

      ticktack = !ticktack;
      time_second = timedate.tm_sec;
    }

    u8g2.sendBuffer(); // transfer internal memory to the display
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
    redirectHome();
  } else {
  if (!authentication()) {
    return server.requestAuthentication();
  }
    server.sendHeader("Cache-Control", "no-cache, no-store");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");

    sendHeader();

    String html = "<h1>Welcome to the Dot Matrix Clock</h1>\n";
    html += "Nothing here.. Go to configuration tab...";
    server.sendContent(html);

    sendFooter();

    server.sendContent("");
    server.client().stop();
  }
}

void handleConfigure() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(notifyLight, LOW);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendHeader();

  String html = "<h1>Configuration</h1>\n";
  html += "<form action='/saveconfig' method='GET'>\n";
  html += "  <h5>Brightness settings</h5>\n";
  html += "  <label for='intensity_clock' class='form-label'>Clock brightness</label>\n";
  html += "  <input type='range' class='form-range' min='0' max='25' value='" + String(INTENSITY_CLOCK) + "' id='intensity_clock' name='intensity_clock'>\n";
  html += "  <label for='intensity_text' class='form-label'>Text brightness</label>\n";
  server.sendContent(html);

  String isChecked = "";
  if (DISPLAY_SECOND_TICK) {
    isChecked = "checked=''";
  }

  html = "  <h5>Other settings</h5>\n";
  html += "  <div class='form-check form-switch'>\n";
  html += "    <input class='form-check-input' type='checkbox' role='switch' value='true' id='display_ticktock' name='display_ticktock' " + isChecked + ">\n";
  html += "    <label class='form-check-label' for='display_ticktock'>Display ticktock pixels for seconds</label>\n";
  html += "  </div>\n";
  html += "  <button type='submit' class='btn btn-primary'>Submit</button>\n";
  html += "</form>\n";
  html += "<br />\n";
  server.sendContent(html);

  html = "<div class='card'>\n";
  html += "  <div class='card-body'>\n";
  html += "    <h5 class='card-title'>Firmware update</h5>\n";
  html += "    <p class='card-text'>You can download a firmware file from <a href='https://github.com/tubit/arduino-led-matrix/releases'>Github</a> and upload it to your clock here.</p>\n";
  html += "    <a href='/update' class='btn btn-primary'>Go!</a>\n";
  html += "  </div>\n";
  html += "</div>\n";
  html += "<br />\n";
  html += "<div class='card'>\n";
  html += "  <div class='card-body'>\n";
  html += "    <h5 class='card-title'>Reset settings</h5>\n";
  html += "    <a href='/forgetwifi' class='btn btn-warning'>Forget WiFi settings</a>&nbsp;\n";
  html += "    <a href='/forgetconfig' class='btn btn-danger'>Delete config</a>\n";
  html += "  </div>\n";
  html += "</div>\n";
  server.sendContent(html);

  sendFooter();

  server.sendContent("");
  server.client().stop();
}

void handleSaveConfig() {
  if (!authentication()) {
    return server.requestAuthentication();
  }

  INTENSITY_CLOCK = server.arg("intensity_clock").toInt();
  if (server.hasArg("display_ticktock")) {
    DISPLAY_SECOND_TICK = true;
  } else {
    DISPLAY_SECOND_TICK = false;
  }

  writeConfig();

  update_matrix = true;
  redirectHome();
}

// reset Wifi settings
void handleForgetWifi() {
  if (!authentication()) {
    return server.requestAuthentication();
  }

  redirectHome();
  resetWifiConfig();
}

// delete config file
void handleForgetConfig() {
  if (!authentication()) {
    return server.requestAuthentication();
  }

  redirectHome();
  LittleFS.remove(CONFIG);

  Serial.println("Config deleted.");

  ESP.restart();
}

void resetWifiConfig() {
    Serial.println("WiFi reset button pressed, will reset WiFi settings now.");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    Serial.print("WiFi settings removed. Release button for reboot.");

    ESP.restart();
}

void redirectHome() {
  // Send them back to the Root Directory
  server.sendHeader("Location", String("/configure"), true);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");
  server.client().stop();
  delay(1000);
}

void writeConfig() {
  File f = LittleFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("File open failed!");
  } else {
    Serial.println("Saving settings now...");
    f.println("INTENSITY_CLOCK=" + String(INTENSITY_CLOCK));
    f.println("DISPLAY_SECOND_TICK=" + String(DISPLAY_SECOND_TICK));
  }
  f.close();

  delay(10);
  readConfig();
}

void readConfig() {
  if (LittleFS.exists(CONFIG) == false) {
    Serial.println("Settings File does not yet exists.");
    writeConfig();
    return;
  }
  File fr = LittleFS.open(CONFIG, "r");
  String line;
  while (fr.available()) {
    line = fr.readStringUntil('\n');
    if (line.indexOf("INTENSITY_ANIMATION=") >= 0) {
      INTENSITY_ANIMATION = line.substring(line.lastIndexOf("INTENSITY_ANIMATION=") + 20).toInt();
      Serial.println("INTENSITY_ANIMATION: " + String(INTENSITY_ANIMATION));
    }
    if (line.indexOf("INTENSITY_CLOCK=") >= 0) {
      INTENSITY_CLOCK = line.substring(line.lastIndexOf("INTENSITY_CLOCK=") + 16).toInt();
      Serial.println("INTENSITY_CLOCK: " + String(INTENSITY_CLOCK));
    }
    if (line.indexOf("INTENSITY_TEXT=") >= 0) {
      INTENSITY_TEXT = line.substring(line.lastIndexOf("INTENSITY_TEXT=") + 15).toInt();
      Serial.println("INTENSITY_TEXT: " + String(INTENSITY_TEXT));
    }
    if (line.indexOf("DISPLAY_SECOND_TICK=") >= 0) {
      DISPLAY_SECOND_TICK = line.substring(line.lastIndexOf("DISPLAY_SECOND_TICK=") + 20).toInt();
      Serial.println("DISPLAY_SECOND_TICK: " + String(DISPLAY_SECOND_TICK));
    }
  }
  fr.close();
}

void sendHeader() {
  String html = "<!doctype html>\n";
  html += "<html lang='en'>\n";
  html += "<head>\n";
  html += "  <!-- Required meta tags -->\n";
  html += "  <meta charset='utf-8'>\n";
  html += "  <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>\n";
  html += "  <style>\n";
  html += "    html { position: relative; min-height: 100%; }\n";
  html += "    body { margin-bottom: 60px; }\n";
  html += "    .footer { position: absolute; bottom: 0; width: 100%; height: 60px; line-height: 60px; background-color: #f5f5f5; }\n";
  html += "    main > .container { padding: 60px 15px 0; }\n";
  html += "  </style>\n";
  server.sendContent(html);
  html += "        <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css' rel='stylesheet' integrity='sha384-rbsA2VBKQhggwzxH7pPCaAqO46MgnOM80zW1RWuH61DGLwZJEdK2Kadq2F9CUG65' crossorigin='anonymous'>\n";
  html += "  <title>Dot-Matrix-Clock!</title>\n";
  html += "</head>\n";
  html += "<body class='d-flex flex-column h-100'>\n";
  html += "  <header>\n";
  html += "    <nav class='navbar navbar-expand-md navbar-dark fixed-top bg-dark'>\n";
  html += "      <div class='container-fluid'>\n";
  html += "        <a class='navbar-brand' href='#'>Matrix Clock</a>\n";
  html += "        <button class='navbar-toggler' type='button' data-bs-toggle='collapse' data-bs-target='#navbarCollapse' aria-controls='navbarCollapse' aria-expanded='false' aria-label='Toggle navigation'>\n";
  html += "          <span class='navbar-toggler-icon'></span>\n";
  html += "        </button>\n";
  html += "        <div class='collapse navbar-collapse' id='navbarCollapse'>\n";
  html += "          <ul class='navbar-nav me-auto mb-2 mb-md-0'>\n";
  html += "            <li class='nav-item'>\n";
  html += "              <a class='nav-link' href='/'>Home</a>\n";
  html += "            </li>\n";
  html += "            <li class='nav-item'>\n";
  html += "              <a class='nav-link' href='/configure'>Configuration</a>\n";
  html += "            </li>\n";
  html += "          </ul>\n";
  html += "        </div>\n";
  html += "      </div>\n";
  html += "    </nav>\n";
  html += "  </header>\n";
  html += "  <main class='flex-shrink-0'>\n";
  html += "    <div class='container'>\n";
  server.sendContent(html);
}

void sendFooter() {
  int8_t rssi = getWifiQuality();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println("%");

  String html = "    </div>\n"; 
  html += "  </main>\n";
  html += "  <footer class='footer mt-auto py-3 bg-light'>\n";
  html += "    <div class='container'>\n";
  html += "      <span class='text-muted'>WiFi Signal: " + String(rssi) + "% -/- Version: " + String(VERSION) + "</span>\n";
  html += "    </div>\n";
  html += "  </footer>\n";
  html += "    <script src='https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js' integrity='sha384-kenU1KFdBIe4zVF0s0G1M5b4hcpxyD9F7jL+jjXkk+Q2h455rYXK/7HAuoJl+0I4' crossorigin='anonymous'></script>\n";
  html += "  </body>\n";
  html += "</html>\n";
  server.sendContent(html);
}
