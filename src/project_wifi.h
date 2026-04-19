#ifndef FUNCTIONS_WIFI_H
#define FUNCTIONS_WIFI_H
//#pragma once
#include <Arduino.h>

#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>


#define WIFI_CONNECT_TIME 15000  // connecting to a network is given 15 seconds
#define WIFI_HOSTNAME "SF-Master"
#define WIFI_AP_SSID "SplitFlap"
#define WIFI_AP_PASS "SomeSecurePassword"
#define WIFI_CHECK_TIME 900000   // will check for WiFi connection every 15 minutes

void connect2WiFi();
bool checkWiFi();

#endif