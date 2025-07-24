#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>
#include <FastLED.h>

// RGB LED Configuration for ESP32-S3-DevKitC-1
#define LED_PIN     48
#define NUM_LEDS    1
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// LED state variables
bool ledState = false;
uint8_t ledBrightness = 100;  // 0-255
CRGB currentColor = CRGB::Red;  // Default color

// AP Mode Credentials
const char* ap_ssid = "LED_CONFIG";
const char* ap_password = "12345678";

// IP Configurations
IPAddress local_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);
Preferences preferences;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

// WiFi credentials storage
String sta_ssid = "";
String sta_password = "";

// Boot button configuration
const int BOOT_BUTTON_PIN = 0;
const unsigned long LONG_PRESS_TIME = 3000;

// Performance optimization constants
const unsigned long WIFI_RETRY_INTERVAL = 30000;
const unsigned long BUTTON_DEBOUNCE_TIME = 50;

// Boot button state variables
bool buttonPressed = false;
unsigned long buttonPressTime = 0;
bool lastButtonState = HIGH;

// Simplified connection status variables
bool connectionAttempted = false;
bool connectedToWiFi = false;

// Performance optimization variables
unsigned long lastButtonCheck = 0;
unsigned long lastScanTime = 0;
int scanResults = -1;

// Connection management variables
bool isConnecting = false;
unsigned long connectionStartTime = 0;
const unsigned long CONNECTION_TIMEOUT = 15000; // 15 seconds

// Function declarations
String getSimplePage(String title, String content);
void updateRGBLED();
void loadLedState();
void saveLedState();
void handleLed();
void handleToggleLed();
void handleSetColor();
void handleSetBrightness();

// Lightweight HTML templates
String getSimplePage(String title, String content) {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>" + title + "</title>";
  html += "<style>";
  // Base styles
  html += "body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0;min-height:100vh;transition:background 0.3s;}";
  html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;transition:background 0.3s,color 0.3s;}";
  html += ".nav{margin-bottom:20px;} .nav a{margin-right:15px;text-decoration:none;color:#007bff;transition:color 0.3s;}";
  html += ".status-success{background:#d4edda;color:#155724;padding:10px;border-radius:5px;margin:10px 0;}";
  html += ".status-danger{background:#f8d7da;color:#721c24;padding:10px;border-radius:5px;margin:10px 0;}";
  html += ".status-warning{background:#fff3cd;color:#856404;padding:10px;border-radius:5px;margin:10px 0;}";
  html += ".network{background:#e9ecef;margin:5px 0;padding:10px;border-radius:5px;cursor:pointer;transition:background 0.3s;}";
  html += ".network:hover{background:#dee2e6;}";
  html += ".selected{background:#28a745;color:white;}";
  html += "input,button{padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;}";
  html += "button{background:#007bff;color:white;cursor:pointer;}";
  html += "a{color:#007bff;text-decoration:none;}";
  html += "a:hover{text-decoration:underline;}";
  // Dark mode toggle styles
  html += ".toggle-container{position:fixed;top:20px;right:20px;z-index:1000;}";
  html += ".toggle-input{width:0;height:0;visibility:hidden;}";
  html += ".toggle-label{width:60px;height:30px;position:relative;display:block;background:#ebebeb;border-radius:30px;box-shadow:inset 0px 2px 8px rgba(0,0,0,0.4);cursor:pointer;transition:0.3s;}";
  html += ".toggle-label:after{content:'';width:26px;height:26px;position:absolute;top:2px;left:2px;background:linear-gradient(180deg,#ffcc89,#d8860b);border-radius:26px;box-shadow:0px 2px 5px rgba(0,0,0,0.2);transition:0.3s;}";
  html += ".toggle-input:checked + .toggle-label{background:#242424;}";
  html += ".toggle-input:checked + .toggle-label:after{left:32px;background:linear-gradient(180deg,#777777,#3a3a3a);}";
  html += ".toggle-label:active:after{width:32px;}";
  // SVG styles for toggle
  html += ".toggle-label svg{position:absolute;width:16px;top:7px;z-index:100;transition:0.3s;}";
  html += ".toggle-label svg.sun{left:6px;fill:#ffffff;}";
  html += ".toggle-label svg.moon{right:6px;fill:#7e7e7e;}";
  html += ".toggle-input:checked + .toggle-label svg.sun{fill:#7e7e7e;}";
  html += ".toggle-input:checked + .toggle-label svg.moon{fill:#ffffff;}";
  // Dark mode styles - using body class approach
  html += "body.dark-mode{background:#1a1a1a;}";
  html += "body.dark-mode .container{background:#2d2d2d;color:#ffffff;}";
  html += "body.dark-mode .nav a{color:#66b3ff;}";
  html += "body.dark-mode .network{background:#404040;color:#ffffff;}";
  html += "body.dark-mode .network:hover{background:#505050;}";
  html += "body.dark-mode input{background:#404040;color:#ffffff;border-color:#666;}";
  html += "body.dark-mode .status-success{background:#155724;color:#d4edda;}";
  html += "body.dark-mode .status-danger{background:#721c24;color:#f8d7da;}";
  html += "body.dark-mode .status-warning{background:#856404;color:#fff3cd;}";
  html += "body.dark-mode button{background:#0d6efd;border-color:#0d6efd;}";
  html += "</style></head><body>";
  // Add the toggle at the top
  html += "<div class='toggle-container'>";
  html += "<input type='checkbox' id='dark-mode' class='toggle-input'/>";
  html += "<label for='dark-mode' class='toggle-label'>";
  html += "<svg class='sun' viewBox='0 0 24 24' fill='none' xmlns='http://www.w3.org/2000/svg'>";
  html += "<path d='M12 3V4M12 20V21M4 12H3M6.31412 6.31412L5.5 5.5M17.6859 6.31412L18.5 5.5M6.31412 17.69L5.5 18.5001M17.6859 17.69L18.5 18.5001M21 12H20M16 12C16 14.2091 14.2091 16 12 16C9.79086 16 8 14.2091 8 12C8 9.79086 9.79086 8 12 8C14.2091 8 16 9.79086 16 12Z' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'/>";
  html += "</svg>";
  html += "<svg class='moon' viewBox='0 0 24 24' fill='none' xmlns='http://www.w3.org/2000/svg'>";
  html += "<path d='M3.32031 11.6835C3.32031 16.6541 7.34975 20.6835 12.3203 20.6835C16.1075 20.6835 19.3483 18.3443 20.6768 15.032C19.6402 15.4486 18.5059 15.6834 17.3203 15.6834C12.3497 15.6834 8.32031 11.654 8.32031 6.68342C8.32031 5.50338 8.55165 4.36259 8.96453 3.32996C5.65605 4.66028 3.32031 7.89912 3.32031 11.6835Z' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'/>";
  html += "</svg>";
  html += "</label>";
  html += "</div>";
  html += "<div class='container'>";
  html += "<div class='nav'>";
  html += "<a href='/'>GLOW by zamil</a>";
  html += "<a href='/wifi-setup'>WiFi Setup</a>";
  html += "<a href='/led'>LED Control</a>";
  html += "<a href='/timer'>Timer</a>";
  html += "<a href='/info'>Info</a>";
  html += "</div>";
  html += content;
  html += "</div>";
  // Add JavaScript to handle dark mode toggle
  html += "<script>";
  html += "const toggle = document.getElementById('dark-mode');";
  html += "const body = document.body;";
  html += "toggle.addEventListener('change', function() {";
  html += "  if (this.checked) {";
  html += "    body.classList.add('dark-mode');";
  html += "    localStorage.setItem('darkMode', 'enabled');";
  html += "  } else {";
  html += "    body.classList.remove('dark-mode');";
  html += "    localStorage.setItem('darkMode', 'disabled');";
  html += "  }";
  html += "});";
  html += "if (localStorage.getItem('darkMode') === 'enabled') {";
  html += "  body.classList.add('dark-mode');";
  html += "  toggle.checked = true;";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  return html;
}

// LED state functions
void loadLedState() {
  preferences.begin("led", true);
  ledState = preferences.getBool("state", false);
  ledBrightness = preferences.getUChar("brightness", 100);
  uint32_t colorValue = preferences.getULong("color", 0xFF0000); // Default red
  currentColor = CRGB(colorValue);
  preferences.end();
  updateRGBLED();
}

void saveLedState() {
  preferences.begin("led", false);
  preferences.putBool("state", ledState);
  preferences.putUChar("brightness", ledBrightness);
  preferences.putULong("color", ((uint32_t)currentColor.r << 16) | ((uint32_t)currentColor.g << 8) | currentColor.b);
  preferences.end();
}

void updateRGBLED() {
  if (ledState) {
    leds[0] = currentColor;
    FastLED.setBrightness(ledBrightness);
  } else {
    leds[0] = CRGB::Black;
  }
  FastLED.show();
}

// LED web handlers
void handleLed() {
  String content = "<h1>RGB LED Control</h1>";
  content += "<p>LED is currently <strong>" + String(ledState ? "ON" : "OFF") + "</strong></p>";
  content += "<p>Brightness: " + String(ledBrightness) + "/255</p>";
  content += "<p>Color: RGB(" + String(currentColor.r) + ", " + String(currentColor.g) + ", " + String(currentColor.b) + ")</p>";
  content += "<form action='/toggle-led' method='POST'><button type='submit'>" + String(ledState ? "Turn OFF" : "Turn ON") + "</button></form>";
  content += "<hr><h3>Color Selection</h3>";
  content += "<form action='/set-color' method='POST'>";
  content += "<button type='submit' name='color' value='red'>Red</button>";
  content += "<button type='submit' name='color' value='green'>Green</button>";
  content += "<button type='submit' name='color' value='blue'>Blue</button>";
  content += "<button type='submit' name='color' value='yellow'>Yellow</button>";
  content += "<button type='submit' name='color' value='purple'>Purple</button>";
  content += "<button type='submit' name='color' value='cyan'>Cyan</button>";
  content += "<button type='submit' name='color' value='white'>White</button>";
  content += "</form>";
  content += "<hr><h3>Brightness Control</h3>";
  content += "<form action='/set-brightness' method='POST'>";
  content += "<input type='range' name='brightness' min='10' max='255' value='" + String(ledBrightness) + "'>";
  content += "<br><button type='submit'>Set Brightness</button>";
  content += "</form>";
  server.send(200, "text/html", getSimplePage("LED Control", content));
}

void handleToggleLed() {
  ledState = !ledState;
  updateRGBLED();
  saveLedState();
  server.sendHeader("Location", "/led");
  server.send(303, "text/plain", "");
}

void handleSetColor() {
  if (server.hasArg("color")) {
    String color = server.arg("color");
    if (color == "red") currentColor = CRGB::Red;
    else if (color == "green") currentColor = CRGB::Green;
    else if (color == "blue") currentColor = CRGB::Blue;
    else if (color == "yellow") currentColor = CRGB::Yellow;
    else if (color == "purple") currentColor = CRGB::Purple;
    else if (color == "cyan") currentColor = CRGB::Cyan;
    else if (color == "white") currentColor = CRGB::White;
    updateRGBLED();
    saveLedState();
  }
  server.sendHeader("Location", "/led");
  server.send(303, "text/plain", "");
}

void handleSetBrightness() {
  if (server.hasArg("brightness")) {
    ledBrightness = server.arg("brightness").toInt();
    if (ledBrightness < 10) ledBrightness = 10;
    if (ledBrightness > 255) ledBrightness = 255;
    updateRGBLED();
    saveLedState();
  }
  server.sendHeader("Location", "/led");
  server.send(303, "text/plain", "");
}

// Function to clear stored credentials
void clearStoredCredentials() {
  Serial.println("Clearing stored WiFi credentials...");
  WiFi.disconnect();
  isConnecting = false;
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  sta_ssid = "";
  sta_password = "";
  connectionAttempted = false;
  connectedToWiFi = false;
  Serial.println("All stored credentials cleared!");
  delay(2000);
  ESP.restart();
}

// Function to force stop all connection attempts
void forceStopConnections() {
  Serial.println("Force stopping all WiFi connections...");
  WiFi.disconnect();
  isConnecting = false;
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  sta_ssid = "";
  sta_password = "";
  connectionAttempted = false;
  connectedToWiFi = false;
  Serial.println("All connections stopped and credentials cleared");
}

// Function to clear credentials via web interface
void handleClearCredentials() {
  Serial.println("Clearing stored credentials via web interface");
  WiFi.disconnect();
  isConnecting = false;
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  sta_ssid = "";
  sta_password = "";
  connectionAttempted = false;
  connectedToWiFi = false;
  String content = "<h1>Credentials Cleared</h1>";
  content += "<div class='status-success'>";
  content += "<p>✅ All WiFi credentials have been cleared successfully.</p>";
  content += "<p><a href='/wifi-setup'>Setup New Connection</a></p>";
  content += "</div>";
  server.send(200, "text/html", getSimplePage("Cleared", content));
}

void handleHome() {
  String content = "<h1>ESP32 WiFi Manager</h1>";
  content += "<p><strong>WiFi Status:</strong> ";
  if (WiFi.status() == WL_CONNECTED) {
    content += "Connected to " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")";
  } else {
    content += "Not connected - <a href='/wifi-setup'>Configure WiFi</a>";
  }
  content += "</p>";
  content += "<p><strong>LED Status:</strong> " + String(ledState ? "ON" : "OFF") + "</p>";
  content += "<p><strong>AP IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
  content += "<p><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  server.send(200, "text/html", getSimplePage("Home", content));
}

void handleWifiSetup() {
  String content = "<h1>WiFi Setup</h1>";
  if (connectionAttempted) {
    if (connectedToWiFi) {
      content += "<div class='status-success'>";
      content += "✅ Connected to " + sta_ssid + " (" + WiFi.localIP().toString() + ")";
      content += "</div>";
      WiFi.mode(WIFI_AP_STA);
      delay(100);
    } else {
      content += "<div class='status-danger'>";
      content += "❌ Failed to connect to " + sta_ssid + ". Check password or network availability.";
      content += "<br><a href='/clear-credentials' style='color:white;text-decoration:underline;'>Clear Stored Credentials</a>";
      content += "</div>";
      WiFi.mode(WIFI_AP_STA);
      delay(100);
    }
  }
  if (sta_ssid.length() > 0 && !connectionAttempted) {
    content += "<div class='status-warning'>";
    content += "<p><strong>Current stored network:</strong> " + sta_ssid;
    content += " <a href='/clear-credentials' style='margin-left:10px;'>Clear</a></p>";
    content += "</div>";
  }
  content += "<div id='networks'>";
  content += "<h3>Available Networks:</h3>";
  content += "<button onclick='loadNetworks()'>Scan Networks</button>";
  content += "<div id='scan-results'>Click 'Scan Networks' to see available WiFi networks</div>";
  content += "</div>";
  content += "<div id='connection-form' style='display:none;'>";
  content += "<h3>Connect to: <span id='selected-ssid'></span></h3>";
  content += "<form action='/connect' method='POST'>";
  content += "<input type='hidden' id='ssid' name='ssid' value=''>";
  content += "<input type='password' id='password' name='password' placeholder='WiFi Password' required style='width:200px;'>";
  content += "<br><button type='submit'>Connect</button>";
  content += "</form>";
  content += "</div>";
  content += "<script>";
  content += "function selectNetwork(ssid) {";
  content += "  document.getElementById('selected-ssid').textContent = ssid;";
  content += "  document.getElementById('ssid').value = ssid;";
  content += "  document.getElementById('connection-form').style.display = 'block';";
  content += "  document.querySelectorAll('.network').forEach(n => n.classList.remove('selected'));";
  content += "  event.target.classList.add('selected');";
  content += "}";
  content += "function loadNetworks() {";
  content += "  document.getElementById('scan-results').innerHTML = 'Scanning...';";
  content += "  fetch('/scan').then(r => r.text()).then(data => {";
  content += "    document.getElementById('scan-results').innerHTML = data;";
  content += "  }).catch(e => {";
  content += "    document.getElementById('scan-results').innerHTML = 'Scan failed. Try again.';";
  content += "  });";
  content += "}";
  content += "</script>";
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  server.send(200, "text/html", getSimplePage("WiFi Setup", content));
}

void handleScan() {
  String content = "";
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    content = "Scanning in progress...";
  } else if (n == WIFI_SCAN_FAILED || n < 0) {
    content = "Scan failed. <button onclick='loadNetworks()'>Try Again</button>";
    WiFi.scanNetworks(true);
  } else if (n == 0) {
    content = "No networks found. <button onclick='loadNetworks()'>Refresh</button>";
  } else {
    content = "<button onclick='loadNetworks()' style='margin-bottom:10px;'>Refresh</button>";
    int maxNetworks = min(n, 10);
    for (int i = 0; i < maxNetworks; ++i) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;
      int rssi = WiFi.RSSI(i);
      String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      content += "<div class='network' onclick='selectNetwork(\"" + ssid + "\")'>";
      content += "<strong>" + ssid + "</strong><br>";
      content += "Signal: " + String(rssi) + " dBm | " + security;
      content += "</div>";
    }
    if (n > 10) {
      content += "<p>Showing first 10 networks of " + String(n) + " found.</p>";
    }
    WiFi.scanDelete();
  }
  if (n != WIFI_SCAN_RUNNING) {
    WiFi.scanNetworks(true);
  }
  server.send(200, "text/plain", content);
}

void handleConnect() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    sta_ssid = server.arg("ssid");
    sta_password = server.arg("password");
    Serial.println("Received new credentials: " + sta_ssid);
    preferences.begin("wifi", false);
    preferences.putString("ssid", sta_ssid);
    preferences.putString("password", sta_password);
    preferences.end();
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    isConnecting = true;
    connectionStartTime = millis();
    connectionAttempted = true;
    connectedToWiFi = false;
    server.send(200, "text/html",
      getSimplePage("Connecting...",
      "<h1>Trying to connect to " + sta_ssid + "</h1>"
      "<p>Please wait 10-15 seconds and refresh the page.</p>"
      "<a href='/wifi-setup'>Back to WiFi Setup</a>"));
  }
}

void handleTimer() {
  String content = "<h1>Current Time</h1>";
  if (WiFi.status() == WL_CONNECTED) {
    content += "<div id='clock' style='font-size:36px;text-align:center;'>Loading...</div>";
    content += "<script>";
    content += "function updateClock() {";
    content += "  const now = new Date();";
    content += "  document.getElementById('clock').textContent = now.toLocaleTimeString();";
    content += "}";
    content += "setInterval(updateClock, 1000);";
    content += "updateClock();";
    content += "</script>";
  } else {
    content += "<div class='status-danger'>";
    content += "<p>Time requires internet connection.</p>";
    content += "<p><a href='/wifi-setup'>Configure WiFi</a></p>";
    content += "</div>";
  }
  server.send(200, "text/html", getSimplePage("Timer", content));
}

void handleInfo() {
  String content = "<h1>System Information</h1>";
  content += "<h3>Station Mode</h3>";
  content += "<p><strong>Status:</strong> " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</p>";
  if (WiFi.status() == WL_CONNECTED) {
    content += "<p><strong>Network:</strong> " + WiFi.SSID() + "</p>";
    content += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
    content += "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  }
  content += "<p><strong>MAC:</strong> " + WiFi.macAddress() + "</p>";
  content += "<h3>Access Point</h3>";
  content += "<p><strong>SSID:</strong> " + String(ap_ssid) + "</p>";
  content += "<p><strong>IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
  content += "<p><strong>Clients:</strong> " + String(WiFi.softAPgetStationNum()) + "</p>";
  content += "<h3>Hardware</h3>";
  content += "<p><strong>LED Pin:</strong> " + String(LED_PIN) + "</p>";
  content += "<p><strong>LED State:</strong> " + String(ledState ? "ON" : "OFF") + "</p>";
  content += "<p><strong>LED Brightness:</strong> " + String(ledBrightness) + "/255</p>";
  content += "<p><strong>LED Color:</strong> RGB(" + String(currentColor.r) + ", " + String(currentColor.g) + ", " + String(currentColor.b) + ")</p>";
  content += "<h3>System</h3>";
  content += "<p><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  content += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
  server.send(200, "text/html", getSimplePage("System Info", content));
}

void handleNotFound() {
  server.send(404, "text/html", getSimplePage("404", "<h1>Page Not Found</h1>"));
}

// Improved WiFi connection management
void handleWifiConnection() {
  static unsigned long lastRetry = 0;
  if (WiFi.getMode() != WIFI_AP_STA) {
    Serial.println("Mode not dual - forcing AP_STA mode");
    WiFi.mode(WIFI_AP_STA);
  }
  if (isConnecting) {
    if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
      Serial.println("Connection timeout - stopping attempts");
      WiFi.disconnect();
      isConnecting = false;
      connectedToWiFi = false;
      WiFi.mode(WIFI_AP_STA);
      Serial.println("Timeout: Restarting AP to ensure it stays alive...");
      WiFi.softAP(ap_ssid, ap_password, 1); // or use 11 if 1 is crowded;
      WiFi.softAPConfig(local_ip, gateway, subnet);
      delay(200);
      Serial.println("AP restarted. IP: " + WiFi.softAPIP().toString());
      connectionAttempted = true;
    }
    return;
  }
}

void maintainDualMode() {
  static unsigned long lastModeCheck = 0;
  if (millis() - lastModeCheck >= 2000) {
    if (WiFi.getMode() != WIFI_AP_STA) {
      Serial.println("Mode drift detected - restoring AP_STA mode");
      WiFi.mode(WIFI_AP_STA);
      delay(100);
    }
    IPAddress apIP = WiFi.softAPIP();
    int apClients = WiFi.softAPgetStationNum();
    if (
      apIP[0] == 0 || apIP[1] == 0 || apIP[2] == 0 || apIP[3] == 0 ||
      apIP.toString() == "0.0.0.0" ||
      apClients < 0
    ) {
      Serial.println("AP not running or lost - force restarting AP");
      WiFi.softAP(ap_ssid, ap_password, 1); // or use 11 if 1 is crowded;
      WiFi.softAPConfig(local_ip, gateway, subnet);
      delay(100);
      Serial.println("AP restarted. IP: " + WiFi.softAPIP().toString());
    }
    lastModeCheck = millis();
  }
}

void handleBootButton() {
  bool currentState = digitalRead(BOOT_BUTTON_PIN);
  if (currentState == LOW && lastButtonState == HIGH) {
    buttonPressed = true;
    buttonPressTime = millis();
  }
  if (currentState == HIGH && lastButtonState == LOW && buttonPressed) {
    unsigned long duration = millis() - buttonPressTime;
    if (duration >= LONG_PRESS_TIME) {
      clearStoredCredentials();
    }
    buttonPressed = false;
  }
  lastButtonState = currentState;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 WiFi Manager...");
  // Initialize FastLED for RGB LED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  loadLedState();
  // Test RGB LED on startup with different colors
  leds[0] = CRGB::Red; FastLED.show(); delay(300);
  leds[0] = CRGB::Green; FastLED.show(); delay(300);
  leds[0] = CRGB::Blue; FastLED.show(); delay(300);
  leds[0] = CRGB::Black; FastLED.show(); delay(300);
  updateRGBLED();
  // Initialize button
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  // Load saved credentials
  preferences.begin("wifi", true);
  sta_ssid = preferences.getString("ssid", "");
  sta_password = preferences.getString("password", "");
  preferences.end();
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.softAP(ap_ssid, ap_password, 1); // or use 11 if 1 is crowded;
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("AP Started:");
  Serial.println("SSID: " + String(ap_ssid));
  Serial.println("IP: " + WiFi.softAPIP().toString());
  Serial.println("Mode: " + String(WiFi.getMode() == WIFI_AP_STA ? "AP_STA" : "Other"));
  if (sta_ssid.length() > 0) {
    Serial.println("Found stored credentials for: " + sta_ssid);
    Serial.println("Will attempt connection...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    isConnecting = true;
    connectionStartTime = millis();
    connectionAttempted = false;
  } else {
    Serial.println("No stored WiFi credentials found");
  }
  // Setup web server
  server.on("/", handleHome);
  server.on("/wifi-setup", handleWifiSetup);
  server.on("/scan", handleScan);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/clear-credentials", handleClearCredentials);
  server.on("/led", handleLed);
  server.on("/toggle-led", HTTP_POST, handleToggleLed);
  server.on("/set-color", HTTP_POST, handleSetColor);
  server.on("/set-brightness", HTTP_POST, handleSetBrightness);
  server.on("/timer", handleTimer);
  server.on("/info", handleInfo);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
  WiFi.scanNetworks(true);
  Serial.println("Setup complete - AP should remain active at all times");
}

void loop() {
  handleBootButton();
  server.handleClient();
  maintainDualMode();
  if (isConnecting) {
    Serial.println("Loop: WiFi mode = " + String(WiFi.getMode()));
    Serial.println("Loop: AP IP = " + WiFi.softAPIP().toString());
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ Connected successfully!");
      isConnecting = false;
      connectedToWiFi = true;
    } else if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
      Serial.println("❌ Connection failed. Re-enabling AP...");
      WiFi.disconnect(true);
      isConnecting = false;
      connectedToWiFi = false;
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(ap_ssid, ap_password, 1);
      WiFi.softAPConfig(local_ip, gateway, subnet);
      delay(300);
      Serial.println("AP restarted at: " + WiFi.softAPIP().toString());
      Serial.println("Loop: WiFi mode after restart = " + String(WiFi.getMode()));
    }
  }
  // if (WiFi.softAPIP().toString() == "0.0.0.0") {
  //   WiFi.softAP(ap_ssid, ap_password, 1); // or use 11 if 1 is crowded;
  //   WiFi.softAPConfig(local_ip, gateway, subnet);
  //   Serial.println("AP check: restored AP");
  // }
}