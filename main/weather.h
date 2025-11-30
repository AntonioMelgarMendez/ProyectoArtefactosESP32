#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "time.h"

extern float internetTempC;
extern String internetDesc;
extern bool internetUpdated;
extern float internetPrecipMm;
extern int internetPrecipProb;
extern float internetWind;
extern int internetWeatherCode;

inline void fetch_open_meteo(double lat, double lon) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(lat, 4) +
               "&longitude=" + String(lon, 4) +
               "&current_weather=true&hourly=precipitation&forecast_days=1&timezone=auto"; 

  if (http.begin(client, url)) {
    int code = http.GET();
    
    if (code == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(8192); 
      DeserializationError err = deserializeJson(doc, payload);
      
      if (!err) {
        if (doc.containsKey("current_weather")) {
          internetTempC = doc["current_weather"]["temperature"].as<float>();
          internetWind = doc["current_weather"]["windspeed"].as<float>();
          internetWeatherCode = doc["current_weather"]["weathercode"].as<int>();
        }
        if (doc.containsKey("hourly")) {
          JsonArray prec = doc["hourly"]["precipitation"];
          struct tm tmnow;
          
          if (getLocalTime(&tmnow)) {
             int hourIdx = tmnow.tm_hour; 
             if (hourIdx < prec.size()) {
                 internetPrecipMm = prec[hourIdx].as<float>();
             }
          } else {
             if (prec.size() > 0) internetPrecipMm = prec[0].as<float>();
          }
        }
        internetUpdated = true;
      }
    }
    http.end();
  }
}

inline void internet_open_meteo_loop(double lat, double lon) {
  static unsigned long last = 0;
  if (last == 0 || millis() - last > 900000UL) { 
    last = millis();
    if (abs(lat) > 0.001) fetch_open_meteo(lat, lon);
  }
}