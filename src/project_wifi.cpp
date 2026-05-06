#include "project_wifi.h"

unsigned long last_wifi_check = 0;

// Scanning States
int scanResult = -2;        // -2 = idle, -1 = scanning, >=0 = number of networks
unsigned long scanStartTime = 0;

// Connection test state
bool isTestingConnection = false;
bool connectionSuccess = false;
String testSSID = "";
String testPass = "";
unsigned long connectStartTime = 0;

AsyncWebServer server(80);
DNSServer dnsServer;

// ====================== Helper Functions ======================
String getSafeFilename(String input) {
    input.replace("/", "_"); input.replace("\\", "_"); input.replace("..", "_");
    input.replace(" ", "_"); input.replace(":", "_"); input.replace("\"", "");
    return input;
}

bool saveWifiConfig(const String& filename, const String& ssid, const String& pass) {
  Serial.println("  portal click: save WiFi config");
    String safe = getSafeFilename(filename);
    if (safe.length() == 0) return false;
    
    String path = "/WiFi/" + safe + ".conf";
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    f.println(ssid);
    f.println(pass);
    f.close();
    Serial.printf("WiFi config saved: %s\n", path.c_str());
    return true;
}

bool deleteWifiConfig(const String& filename) {
    String safe = getSafeFilename(filename);
    return LittleFS.remove("/WiFi/" + safe + ".conf");
}

// ====================== ASYNC SCAN ======================
void startWifiScan() {
    if (scanResult == -1) return;   // already scanning

    // Important: clean previous scan and disconnect first
    WiFi.scanDelete();
    delay(50);

    WiFi.disconnect(true);   // force clean state
    delay(100);

    WiFi.scanNetworks(true, false);   // async, show hidden
    scanResult = -1;
    scanStartTime = millis();
    Serial.println("Async WiFi scan started (2nd+ attempt safe version)");
}

String getScanResultsJSON() {
    int n = WiFi.scanComplete();
    
    if (n == WIFI_SCAN_RUNNING) {
        return "{\"status\":\"scanning\"}";
    }
    
    if (n < 0) {  // failed or not started
        return "{\"status\":\"failed\"}";
    }
    
    // Scan done
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i) != "") {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
      }
        
    }
    json += "]";
    
    WiFi.scanDelete();   // clean up
    scanResult = n;
    return json;
}

String listConfigsJSON() {
    String json = "[";
    bool first = true;

    // Ensure directory exists
    if (!LittleFS.exists("/WiFi")) {
        LittleFS.mkdir("/WiFi");
        return "[]";
    }

    File root = LittleFS.open("/WiFi");
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open /WiFi directory");
        return "[]";
    }

    File file = root.openNextFile();
    while (file) {
        String filename = String(file.name());
        
        if (!file.isDirectory() && filename.endsWith(".conf")) {
            String name = filename;
            name.replace(".conf", "");

            // Read SSID from first line
            File confFile = LittleFS.open("/WiFi/" + filename, "r");
            String ssid = "???";
            if (confFile) {
                ssid = confFile.readStringUntil('\n');
                ssid.trim();
                confFile.close();
            }

            if (!first) json += ",";
            json += "{\"name\":\"" + name + "\",\"ssid\":\"" + ssid + "\"}";
            first = false;
        }
        
        file = root.openNextFile();
        delay(1);                    // Yield to prevent WDT / lag
    }

    json += "]";
    return json;
}

// ====================== TEST CONNECTION ======================
void startConnectionTest(const String& ssid, const String& pass) {
    if (isTestingConnection) return;
    
    testSSID = ssid;
    testPass = pass;
    isTestingConnection = true;
    connectionSuccess = false;
    connectStartTime = millis();
    
    // Important: Stay in AP mode during test
    WiFi.scanDelete();
    WiFi.disconnect(true);     // disconnect from any previous STA
    delay(200);
    
    // Do NOT change to STA yet - just try to connect while still in AP+STA combo
    WiFi.mode(WIFI_AP_STA);    // Allow both modes temporarily
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    Serial.printf("Testing connection to: %s (still in AP mode)\n", ssid.c_str());
}

String getConnectionStatusJSON() {
    if (!isTestingConnection) {
        return "{\"status\":\"idle\"}";
    }

    wl_status_t status = WiFi.status();
    
    if (status == WL_CONNECTED) {
        isTestingConnection = false;
        connectionSuccess = true;
        
        Serial.println("Connection successful - Switching from AP to STA only");
        
        // Now switch to STA only mode
        WiFi.mode(WIFI_STA);
        // WiFi.softAPdisconnect(true);   // optional: fully disable AP
        
        return "{\"status\":\"success\", \"ip\":\"" + WiFi.localIP().toString() + "\"}";
    }
    
    // Timeout after 25 seconds
    if (millis() - connectStartTime > 25000) {
        WiFi.disconnect(true);
        isTestingConnection = false;
        connectionSuccess = false;
        
        // Stay in AP mode on failure
        WiFi.mode(WIFI_AP);
        Serial.println("Connection test failed - staying in AP mode");
        
        return "{\"status\":\"timeout\"}";
    }
    
    return "{\"status\":\"connecting\"}";
}

// WiFi Routines
void connect2WiFi() {
  // Search for WiFi config files
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  String WiFi_Path = "/WiFi/";
  File Wifi_conf = LittleFS.open(WiFi_Path);
  if (!Wifi_conf) {
    Serial.println("Failed to open WiFi conf directory");
  } else if (!Wifi_conf.isDirectory()) {
    Serial.println("Wifi conf dir not directory");
  } else {
    File Wifi_conf_file = Wifi_conf.openNextFile();
    int n_SSIDs = WiFi.scanNetworks();
    if (n_SSIDs > 0) {
      while (Wifi_conf_file) {
        String conf_name = String(Wifi_conf_file.name());
        if (!Wifi_conf_file.isDirectory() && conf_name.endsWith(".conf")) {
          Serial.print("Found WiFi Config File: ");
            Serial.println(conf_name);
          // Read file contents
          // Line 1: AP name
          String AP_name = Wifi_conf_file.readStringUntil('\n');
          const char* AP_name_char = AP_name.c_str();
          //Serial.print("AP Name: "); Serial.println(AP_name_char);
          // Line 2: AP password
          String AP_pass = Wifi_conf_file.readStringUntil('\n');
          const char* AP_pass_char = AP_pass.c_str();
          //Serial.print("AP Pass: "); Serial.println(AP_pass_char);
          
          /*
          if (AP_name == "Oliver") {
            Serial.println("Skipping AP 'Oliver'");
            Wifi_conf_file = Wifi_conf.openNextFile();
            continue;
          }
          */

          // Check if AP exists
          Serial.print("Searching for AP: ");
            Serial.print(AP_name);
            //Serial.println();
          bool AP_exists = false;
          for (int i = 0; i < n_SSIDs; ++i) {
            //Serial.print(" AP: "); Serial.println(WiFi.SSID(i));
            if (WiFi.SSID(i) == AP_name) {
              AP_exists = true;
              Serial.println(" Found!");
              break;
            }
          }
          if (!AP_exists) {
            Serial.println(" not found!");
          }

          if (AP_exists) {
            // Attempt to connect
            Serial.print("Connecting to AP: ");
              Serial.print(AP_name);
              Serial.print(" ");
            WiFi.begin(AP_name_char,AP_pass_char);
            unsigned long connect_start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - connect_start <= WIFI_CONNECT_TIME) {
              Serial.print(".");
              delay(100);
            }
            if (WiFi.status() == WL_CONNECTED) {
              Serial.println(" Success!");
              last_wifi_check = millis();
              break; // break out of file loop
            } else {
              Serial.print(" Failed!");
            }
          }
        }
        Wifi_conf_file = Wifi_conf.openNextFile();
      }
    }
    #ifdef WIFI_FALLBACK_AP
      Serial.print("WiFi.status = "); Serial.println(WiFi.status());
      if (WiFi.status() != WL_CONNECTED) {
        startCaptivePortal();
      } else {
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      }
    #else
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      }
    #endif
  }
}

bool checkWiFi() {
  bool ran_wifi = false;
  if (last_wifi_check + WIFI_CHECK_TIME <= millis()) {
    ran_wifi = true;
    Serial.print("Checking for WiFi: ");
    if (!WL_CONNECTED) {
      Serial.println("Oh no! WiFi is not connected!");
      // attempt to reconnect to WiFi
        // if unsuccessful, attempt to connect to know nnetworks
          // if still unsuccessful, revert to AP
      // What else needs to happen?
        // reconnect to udp/ntp?
    } else {
      Serial.println(" ... Good!");
    }
    last_wifi_check = millis();
  }
  return ran_wifi;
}

// ====================== Routes ======================
void setupWebServer() {
    // Main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/wifi_config.html", "text/html");
    });

    // Start scan
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        startWifiScan();
        request->send(200, "text/plain", "Scan started");
    });

    // Get results (polling)
    server.on("/scanresult", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getScanResultsJSON());
    });

    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = listConfigsJSON();
        request->send(200, "application/json", json);
    });

    // Test connection before saving
    server.on("/testconnect", HTTP_POST, 
        [](AsyncWebServerRequest *request){}, 
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                String body = String((char*)data).substring(0, len);
                
                // Parse JSON
                int sStart = body.indexOf("\"ssid\":\"") + 8;
                int sEnd = body.indexOf("\"", sStart);
                String ssid = body.substring(sStart, sEnd);
                
                int pStart = body.indexOf("\"pass\":\"") + 8;
                int pEnd = body.indexOf("\"", pStart);
                String pass = body.substring(pStart, pEnd);

                startConnectionTest(ssid, pass);
                request->send(200, "text/plain", "Connection test started");
            }
        }
    );

    // Check connection status
    server.on("/connectstatus", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getConnectionStatusJSON());
    });

    // Save
    server.on("/save", HTTP_POST, 
        [](AsyncWebServerRequest *request){}, 
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String body = String((char*)data).substring(0, len);
            
            // Simple JSON extraction (upgrade to ArduinoJson later if desired)
            String filename = body.substring(body.indexOf("\"filename\":\"") + 12);
            filename = filename.substring(0, filename.indexOf("\""));
            
            String ssid = body.substring(body.indexOf("\"ssid\":\"") + 8);
            ssid = ssid.substring(0, ssid.indexOf("\""));
            
            String pass = body.substring(body.indexOf("\"pass\":\"") + 8);
            pass = pass.substring(0, pass.indexOf("\""));

            if (saveWifiConfig(filename, ssid, pass)) {
                request->send(200, "text/plain", "Configuration saved successfully!");
            } else {
                request->send(500, "text/plain", "Failed to save file.");
            }
        }
    );

    server.on("/attempt", HTTP_POST, 
        [](AsyncWebServerRequest *request){}, 
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                String body = String((char*)data).substring(0, len);
                
                // Extract filename
                int start = body.indexOf("\"filename\":\"") + 12;
                int end = body.indexOf("\"", start);
                String filename = body.substring(start, end);

                // Load credentials from file
                String path = "/WiFi/" + getSafeFilename(filename) + ".conf";
                File f = LittleFS.open(path, "r");
                if (!f) {
                    request->send(404, "text/plain", "Config not found");
                    return;
                }

                String ssid = f.readStringUntil('\n'); ssid.trim();
                String pass = f.readStringUntil('\n'); pass.trim();
                f.close();

                if (ssid.length() == 0) {
                    request->send(400, "text/plain", "Invalid config");
                    return;
                }

                startConnectionTest(ssid, pass);
                request->send(200, "text/plain", "Connection attempt started");
            }
        }
    );

    // Delete
    server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("file")) {
            String f = request->getParam("file")->value();
            if (deleteWifiConfig(f)) {
                request->send(200, "text/plain", "Configuration deleted.");
            } else {
                request->send(500, "text/plain", "Delete failed.");
            }
        } else {
            request->send(400, "text/plain", "Missing file parameter.");
        }
    });

    server.begin();
}

// Call this when you start AP mode
void startCaptivePortal() {
  Serial.println("Starting Access Point");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("MatrixClock-Setup", "12345678");   // ← Change password if you want

  dnsServer.start(53, "*", WiFi.softAPIP());

  setupWebServer();

  // Redirect all unknown domains to our IP (captive portal magic)
  server.onNotFound([](AsyncWebServerRequest *request){
      if (request->host() != WiFi.softAPIP().toString()) {
          request->redirect("http://" + WiFi.softAPIP().toString() + "/");
      } else {
          request->send(404, "text/plain", "Not found");
      }
  });

  //setupWebServer();   // your routes above
  Serial.println("Captive Portal Active → " + WiFi.softAPIP().toString());
}