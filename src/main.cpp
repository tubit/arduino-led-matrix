#include "configuration.h"
#include "utf8ascii.h"
#include "effects.h"

#define VERSION "1.0.1"
#define CONFIG "/config.txt"
#define HOSTNAME "ESP-"

// Change the externalLight to the pin you wish to use if other than the Built-in LED
int notifyLight = LED_BUILTIN; // LED_BUILTIN is is the built in LED on the Wemos
int buttonWifiReset = 16;

/* declare functions */
void flashLED(int count, int delayTime);
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

void scrollText(String text);
void printText(String text);
void printText(int16_t startCol, String text);
void centerText(String text);
/* end declare functions */

ESP8266WebServer server(WEBSERVER_PORT);
ESP8266HTTPUpdateServer serverUpdater;
int16_t hold_seconds = 0;

// for message scrolling
String message = "Hello World";
bool newMessageAvailable = true;

int animation = 0;

bool update_matrix = false;

// clock data
uint8_t time_hour = 255;
uint8_t time_minute = 255;
uint8_t time_second = 255;

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
  mx.clear();
  Serial.println("Matrix created");
  mx.control(MD_MAX72XX::INTENSITY, 0);
  printText("moin!");

  for (int inx = 0; inx <= 15; inx++) {
    mx.control(MD_MAX72XX::INTENSITY, inx);
    delay(100);
  }
  
  // start WifiManager, set configCallbacks
  WiFiManager wifiManager;
  //wifiManager.resetSettings();

  // reset WiFi settings
  if (digitalRead(buttonWifiReset)) {
    resetWifiConfig();
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

  // Scroll your own hostname after first boot.
  message = WiFi.getHostname();
  newMessageAvailable = true;

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

  if (DISPLAY_SECOND_TICK && time_second != timedate.tm_sec) {
    mx.setPoint(0, 0, ticktack);
    mx.setPoint(1, 0, !ticktack);
    ticktack = !ticktack;

    time_second = timedate.tm_sec;
  }

  if (digitalRead(buttonWifiReset)) {
    hold_seconds++;
    // calculate row and column based on the counter and the column count of the display.
    mx.setPoint(hold_seconds / mx.getColumnCount(), mx.getColumnCount() - (hold_seconds - (hold_seconds / mx.getColumnCount() * mx.getColumnCount())), 1);
    
    if (hold_seconds > mx.getColumnCount() * ROW_SIZE) {
      hold_seconds = 0;
      mx.clear();
    }
  } else {
    // only reset wifi after at least a few seconds of hold the button
    if (hold_seconds > mx.getColumnCount() * 2) {
      resetWifiConfig();
    } else if (hold_seconds > 0) {
      hold_seconds = 0;
      update_matrix = true;
      ESP.restart();
    } 
  }

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }
  if (ENABLE_OTA) {
    ArduinoOTA.handle();
  }

  if (newMessageAvailable) {
    Serial.println("Processing message: " + String(message));
    mx.control(MD_MAX72XX::INTENSITY, INTENSITY_TEXT);
    scrollText(message);
    newMessageAvailable = false;
    update_matrix = true;
  }

  if (animation > 0) {
    mx.control(MD_MAX72XX::INTENSITY, INTENSITY_ANIMATION);
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

    mx.control(MD_MAX72XX::INTENSITY, INTENSITY_CLOCK);
    centerText(current_time);
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
  scrollText("Please Connect to AP: " + String(myWiFiManager->getConfigPortalSSID()));
  printText("config");
}

void scrollText(String text) {
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }

  mx.clear();

  for (unsigned int idx = 0; idx < text.length(); idx++) {
    charWidth = mx.getChar(utf8ascii(text.charAt(idx)), sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i <= charWidth; i++) { // allow space between characters 
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(SCROLL_DELAY_MS);
    }

    if (WEBSERVER_ENABLED) {
      server.handleClient();
    }
  }

  // fill the screen with blank to scroll the message out of the screen
  for (unsigned int idx = 0; idx < MAX_DEVICES * COL_SIZE; idx++) {
    mx.transform(MD_MAX72XX::TSL);
    delay(SCROLL_DELAY_MS);

    if (idx % COL_SIZE == 0 && WEBSERVER_ENABLED) {
      server.handleClient();
    }
  }
}

void printText(int16_t startCol, String text) {
  int16_t col = (MAX_DEVICES * COL_SIZE) - 1;
  int8_t charWidth = 8;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts

  mx.clear();

  // if the startColumn is not bigger then the maximum available columns, start there
  if (startCol <= col) {
    col = startCol;
  }

  // print each char, but stop if it is not fitting
  for (unsigned int idx = 0; idx < text.length() && col >= 0; idx++) {
    mx.setChar(col, text.charAt(idx));

    charWidth = mx.getChar(utf8ascii(text.charAt(idx)), sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
    col = col - CHAR_SPACING - charWidth;
  }
}

void printText(String text) {
  printText((MAX_DEVICES * COL_SIZE) - 1, text);
}

void centerText(String text) {
  int16_t maxCol = (MAX_DEVICES * COL_SIZE) - 1;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts
  int8_t charWidth = 8;
  int8_t textWidth = 0;

  for (unsigned int idx = 0; idx < text.length() && textWidth <= maxCol; idx++) {
    charWidth = mx.getChar(utf8ascii(text.charAt(idx)), sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
    textWidth += charWidth;

    if (idx < text.length() - 1) {
      textWidth += CHAR_SPACING;
    }
  }

  int startAt = maxCol - ((maxCol - textWidth) / 2) + 1;
  printText(startAt, text);
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
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i) == "message") {
        message = server.arg(i);
        newMessageAvailable = true;
      } else if (server.argName(i) == "animation") {
        animation = server.arg(i).toInt();
      }
    }
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
    html += "<form action='/' method='POST'>\n";
    html += "  <div class='form-group'>\n";
    html += "    <label for='scrollText'>Scroll message:</label><br />\n";
    html += "    <input id='scrollText' type='text' class='form-control' name='message' value='' maxlength='60' placeholder='Text to scroll'>\n";
    html += "    <button type='submit'  class='btn btn-primary' name='submit'>Send</button>\n";
    html += "  </div>";
    html += "</form><br />\n";
    html += "<form action='/' method='POST'>\n";
    html += "  <div class='form-group'>\n";
    html += "    <label id='animationSelect'>Animation:</label>\n";
    html += "    <select class='form-control' id='animationSelect' name='animation'>\n";
    html += "      <option value='1'>Bullseye</option>\n";
    html += "      <option value='2'>Jumping dot</option>\n";
    html += "      <option value='3'>Cross</option>\n";
    html += "      <option value='4'>Diagonal</option>\n";
    html += "      <option value='5'>Spiral</option>\n";
    html += "      <option value='6'>Up-Down</option>\n";
    html += "      <option value='7'>Left-Right</option>\n";
    html += "      <option value='8'>Chessboard</option>\n";
    html += "      <option value='9'>:-)</option>\n";
    html += "    </select>\n";
    html += "    <button type='submit'  class='btn btn-primary' name='submit'>Send</button>\n";
    html += "  </div>";
    html += "</form>\n";
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
  html += "  <input type='range' class='form-range' min='0' max='15' value='" + String(INTENSITY_CLOCK) + "' id='intensity_clock' name='intensity_clock'>\n";
  html += "  <label for='intensity_text' class='form-label'>Text brightness</label>\n";
  html += "  <input type='range' class='form-range' min='0' max='15' value='" + String(INTENSITY_TEXT) + "' id='intensity_text' name='intensity_text'>\n";
  html += "  <label for='intensity_animation' class='form-label'>Animation brightness</label>\n";
  html += "  <input type='range' class='form-range' min='0' max='15' value='" + String(INTENSITY_ANIMATION) + "' id='intensity_animation' name='intensity_animation'>\n";  
  html += "  <label for='scroll_delay_ms' class='form-label'>Scroll delay in milliseconds</label>\n";
  html += "  <input type='range' class='form-range' min='50' max='250' value='" + String(SCROLL_DELAY_MS) + "' id='scroll_delay_ms' name='scroll_delay_ms'>\n";  
  
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

  INTENSITY_ANIMATION = server.arg("intensity_animation").toInt();
  INTENSITY_CLOCK = server.arg("intensity_clock").toInt();
  INTENSITY_TEXT = server.arg("intensity_text").toInt();
  if (server.hasArg("display_ticktock")) {
    DISPLAY_SECOND_TICK = true;
  } else {
    DISPLAY_SECOND_TICK = false;
  }
  SCROLL_DELAY_MS = server.arg("scroll_delay_ms").toInt();

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
  scrollText("config deleted");

  ESP.restart();
}

void resetWifiConfig() {
    Serial.println("WiFi reset button pressed, will reset WiFi settings now.");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    scrollText("WiFi settings reset");
    printText("release");
    Serial.print("WiFi settings removed. Release button for reboot.");

    ESP.restart();
}

void redirectHome() {
  // Send them back to the Root Directory
  server.sendHeader("Location", String("/"), true);
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
    f.println("INTENSITY_ANIMATION=" + String(INTENSITY_ANIMATION));
    f.println("INTENSITY_CLOCK=" + String(INTENSITY_CLOCK));
    f.println("INTENSITY_TEXT=" + String(INTENSITY_TEXT));
    f.println("DISPLAY_SECOND_TICK=" + String(DISPLAY_SECOND_TICK));
    f.println("SCROLL_DELAY_MS=" + String(SCROLL_DELAY_MS));
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
    if (line.indexOf("SCROLL_DELAY_MS=") >= 0) {
      SCROLL_DELAY_MS = line.substring(line.lastIndexOf("SCROLL_DELAY_MS=") + 16).toInt();
      Serial.println("SCROLL_DELAY_MS: " + String(SCROLL_DELAY_MS));
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
