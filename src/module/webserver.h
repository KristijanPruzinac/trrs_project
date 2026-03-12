#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "eeprom.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "../definitions.h"

struct GpsData {
  float lat;
  float lng;
  int   sat;
  bool  fix;
};

extern volatile GpsData gpsData;
extern SemaphoreHandle_t gpsMutex;

// ── Callback type: called when user saves a location ────────
typedef void (*SaveLocationCallback)(float lat, float lng);

// ── Web server API ──────────────────────────────────────────
void webServerInit(const char *ssid, const char *password);
void webServerTask(void *param);
void webServerOnSave(SaveLocationCallback cb);
void webServerUpdateGps(float lat, float lng, int sat, bool fix);
