#include "project_wifi.h"

unsigned long last_wifi_check = 0;

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
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
    } else {
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
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