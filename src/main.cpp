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
uint8_t ledBrightness = 100; // 0-255
CRGB currentColor = CRGB::Red; // Default color

// AP Mode Credentials
const char* ap_ssid = "LED_CONFIG";
const char* ap_password = "12345678";

// IP Configurations
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);
Preferences preferences;

const char* ntpServer = "pool.ntp.org";

// WiFi credentials storage
String sta_ssid = "";
String sta_password = "";

// Boot button configuration
const int BOOT_BUTTON_PIN = 0;
const unsigned long LONG_PRESS_TIME = 3000;
unsigned long buttonPressTime = 0;
bool lastButtonState = HIGH;

// --- REFACTORED State Management Variables ---
bool isConnecting = false;
unsigned long connectionStartTime = 0;
const unsigned long CONNECTION_TIMEOUT = 15000; // 15 seconds
bool connectionAttempted = false;
bool connectedToWiFi = false;


// Function declarations
String getSimplePage(String title, String content);
void updateRGBLED();
void loadLedState();
void saveLedState();
void handleLed();
void handleToggleLed();
void handleSetColor();
void handleSetBrightness();
void handleClearCredentials();

// Lightweight HTML templates (Same as yours, no changes needed here but using F() macro is recommended)
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

// --- LED state functions (No changes needed) ---
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

// --- Web Handlers (Mostly unchanged, using F() macro for memory) ---

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
  server.sendHeader("Location", "/led", true);
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
  server.sendHeader("Location", "/led", true);
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
  server.sendHeader("Location", "/led", true);
  server.send(303, "text/plain", "");
}

void clearStoredCredentials() {
  Serial.println("Clearing stored WiFi credentials...");
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  Serial.println("Credentials cleared. Restarting...");
  delay(1000);
  ESP.restart();
}

void handleClearCredentials() {
  // Disconnect from WiFi before clearing
  WiFi.disconnect(true);
  delay(1000);
  
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
  content += "<p>The device will now only operate in Access Point mode.</p>";
  content += "<p><a href='/wifi-setup'>Setup New Connection</a></p>";
  content += "</div>";
  
  server.send(200, "text/html", getSimplePage("Cleared", content));
}


void handleHome() {
  String content = "<h1>ESP32 WiFi Manager</h1>";
  if (WiFi.status() == WL_CONNECTED) {
    content += "<p><strong>WiFi Status:</strong> Connected to " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")</p>";
  } else {
    content += "<p><strong>WiFi Status:</strong> Not connected - <a href='/wifi-setup'>Configure WiFi</a></p>";
  }
  content += "<p><strong>LED Status:</strong> " + String(ledState ? "ON" : "OFF") + "</p>";
  content += "<p><strong>AP IP Address:</strong> " + WiFi.softAPIP().toString() + "</p>";
  server.send(200, "text/html", getSimplePage("Home", content));
}

void handleWifiSetup() {
    String content = "<h1>WiFi Setup</h1>";

    if (connectionAttempted) {
        if (connectedToWiFi) {
            content += "<div class='status-success'>✅ Connected to <strong>" + sta_ssid + "</strong> (" + WiFi.localIP().toString() + ")</div>";
        } else {
            content += "<div class='status-danger'>❌ Failed to connect to <strong>" + sta_ssid + "</strong>. Please check the password and try again.</div>";
        }
    }

    if (sta_ssid.length() > 0) {
        content += "<div class='status-warning'><p>Current stored network: <strong>" + sta_ssid + "</strong> <a href='/clear-credentials' style='margin-left:10px;'>Clear Credentials</a></p></div>";
    }

    content += "<h3>Available Networks</h3>";
    content += "<button onclick='loadNetworks()'>Scan for Networks</button>";
    content += "<div id='scan-results' style='margin-top:10px;'></div>";
    
    content += "<div id='connection-form' style='display:none; margin-top:20px;'>";
    content += "<h3>Connect to <span id='selected-ssid'></span></h3>";
    content += "<form action='/connect' method='POST'>";
    content += "<input type='hidden' id='ssid' name='ssid'>";
    content += "<input type='password' name='password' placeholder='WiFi Password' required>";
    content += "<button type='submit'>Connect</button>";
    content += "</form></div>";
    
    content += "<script>";
    content += "function selectNetwork(ssid){document.getElementById('selected-ssid').textContent=ssid;document.getElementById('ssid').value=ssid;document.getElementById('connection-form').style.display='block';document.querySelectorAll('.network').forEach(n=>n.classList.remove('selected'));event.currentTarget.classList.add('selected');}";
    content += "function loadNetworks(){const e=document.getElementById('scan-results');e.innerHTML='Scanning...';fetch('/scan').then(r=>r.text()).then(d=>e.innerHTML=d).catch(err=>e.innerHTML='Scan failed.');}";
    content += "</script>";

    server.send(200, "text/html", getSimplePage("WiFi Setup", content));
}

void handleScan() {
    String content = "";
    int n = WiFi.scanNetworks();
    if (n == 0) {
        content = "No networks found.";
    } else {
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
            String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
            content += "<div class='network' onclick='selectNetwork(\"" + ssid + "\")'>";
            content += "<strong>" + ssid + "</strong> (" + String(rssi) + " dBm) - " + security;
            content += "</div>";
        }
    }
    server.send(200, "text/plain", content);
}


void handleConnect() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    sta_ssid = server.arg("ssid");
    sta_password = server.arg("password");

    Serial.println("New credentials received for: " + sta_ssid);
    preferences.begin("wifi", false);
    preferences.putString("ssid", sta_ssid);
    preferences.putString("password", sta_password);
    preferences.end();
    
    // --- FIX: Start the connection attempt HERE, only ONCE ---
    isConnecting = true;
    connectionStartTime = millis();
    connectionAttempted = true; 
    connectedToWiFi = false;
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    Serial.println("Connection attempt started immediately.");

    // Send a response page immediately
    String content = "<h1>Connecting...</h1>";
    content += "<p>Attempting to connect to <strong>" + sta_ssid + "</strong>. The device will connect within 15 seconds.</p>";
    content += "<p>If connection succeeds, you can access this device at its new IP address. If it fails, the configuration AP will remain active.</p>";
    content += "<p>Page will redirect in 5 seconds...</p>";
    content += "<script>setTimeout(() => { window.location.href = '/wifi-setup'; }, 5000);</script>";
    server.send(200, "text/html", getSimplePage("Connecting", content));
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleInfo() {
  String content = "<h1>System Information</h1>";
  content += "<h3>Station Mode</h3>";
  content += "<p><strong>Status:</strong> " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</p>";
  if (WiFi.status() == WL_CONNECTED) {
    content += "<p><strong>Network:</strong> " + WiFi.SSID() + "</p>";
    content += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  }
  content += "<h3>Access Point</h3>";
  content += "<p><strong>SSID:</strong> " + String(ap_ssid) + "</p>";
  content += "<p><strong>IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
  content += "<h3>System</h3>";
  content += "<p><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  content += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
  server.send(200, "text/html", getSimplePage("Info", content));
}


void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void handleBootButton() {
  bool currentState = digitalRead(BOOT_BUTTON_PIN);
  if (currentState == LOW && lastButtonState == HIGH) { // Button just pressed
    buttonPressTime = millis();
  }
  if (currentState == HIGH && lastButtonState == LOW) { // Button just released
    if (millis() - buttonPressTime >= LONG_PRESS_TIME) {
      clearStoredCredentials();
    }
  }
  lastButtonState = currentState;
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting up...");

  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  loadLedState();
  // Test LED
  leds[0] = CRGB::Red; FastLED.show(); delay(200);
  leds[0] = CRGB::Green; FastLED.show(); delay(200);
  leds[0] = CRGB::Blue; FastLED.show(); delay(200);
  updateRGBLED();

  // Initialize boot button
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Load saved credentials
  preferences.begin("wifi", true);
  sta_ssid = preferences.getString("ssid", "");
  sta_password = preferences.getString("password", "");
  preferences.end();
  
  // ALWAYS start in dual mode. This is crucial.
  WiFi.mode(WIFI_AP_STA);
  
  // Start the Access Point
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.print("AP Started. IP Address: ");
  Serial.println(WiFi.softAPIP());

  // If we have credentials, start the connection process
  if (sta_ssid.length() > 0) {
    Serial.println("Found stored credentials for: " + sta_ssid);
    isConnecting = true;
    connectionStartTime = millis();
    // --- FIX: Start the connection attempt HERE, only ONCE ---
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str()); 
  } else {
    Serial.println("No stored WiFi credentials.");
  }

  // Setup web server routes
  server.on("/", HTTP_GET, handleHome);
  server.on("/wifi-setup", HTTP_GET, handleWifiSetup);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/clear-credentials", HTTP_GET, handleClearCredentials);
  server.on("/led", HTTP_GET, handleLed);
  server.on("/toggle-led", HTTP_POST, handleToggleLed);
  server.on("/set-color", HTTP_POST, handleSetColor);
  server.on("/set-brightness", HTTP_POST, handleSetBrightness);
  server.on("/info", HTTP_GET, handleInfo);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web server started.");
}

// --- MAIN LOOP ---
void loop() {
  handleBootButton();
  server.handleClient();

  // --- Centralized WiFi Connection Monitoring ---
  if (isConnecting) {
    // Check for successful connection
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("✅ WiFi Connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      isConnecting = false;
      connectedToWiFi = true;
      configTime(19800, 0, ntpServer); // GMT+5:30 = 19800 seconds
    }
    // Check for connection timeout
    else if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
      Serial.println("❌ Connection failed (timeout).");
      isConnecting = false;
      connectedToWiFi = false;
      
      // Disconnect cleanly and forcefully restart the AP to ensure it's available.
      WiFi.disconnect(true); 
      delay(100);
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(ap_ssid, ap_password);
      WiFi.softAPConfig(local_ip, gateway, subnet);
      Serial.println("AP has been re-initialized to ensure availability.");
    }
  }

  // Optional: Periodically check if the AP has died for any reason and restart it
  static unsigned long lastApCheck = 0;
  if (millis() - lastApCheck > 5000) { // Check every 5 seconds
      if (WiFi.softAPIP() == IPAddress(0,0,0,0)) {
          Serial.println("AP has disappeared! Restarting it...");
          WiFi.softAP(ap_ssid, ap_password);
          WiFi.softAPConfig(local_ip, gateway, subnet);
      }
      lastApCheck = millis();
  }
}
