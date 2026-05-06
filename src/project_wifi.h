#ifndef FUNCTIONS_WIFI_H
#define FUNCTIONS_WIFI_H
//#pragma once
#include <Arduino.h>

#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

extern AsyncWebServer server;
extern DNSServer dnsServer;

void setupWebServer();
void startCaptivePortal();
String scanNetworksJSON();
String listConfigsJSON();

#define WIFI_CONNECT_TIME 15000  // connecting to a network is given 15 seconds
#define WIFI_HOSTNAME "MatrixClock"

#define WIFI_CHECK_TIME 900000   // will check for WiFi connection every 15 minutes

#define WIFI_FALLBACK_AP
#ifdef WIFI_FALLBACK_AP
  #define WIFI_AP_SSID "MatrixClockConnect"
  #define WIFI_AP_PASS "SomeSecurePassword"
#endif

void connect2WiFi();
bool checkWiFi();

#endif