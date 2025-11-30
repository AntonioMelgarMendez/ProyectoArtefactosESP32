#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

extern float internetTempC;
extern String internetDesc;
extern bool internetUpdated;
// nuevos campos
extern float internetPrecipMm;
extern int internetPrecipProb;
extern float internetWind;
extern int internetWeatherCode;

// obtiene current_weather + hourly precipitation & probability (Open‑Meteo, sin key)
inline void fetch_open_meteo(double lat, double lon) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(lat,6) +
               "&longitude=" + String(lon,6) +
               "&current_weather=true&hourly=precipitation,precipitation_probability&timezone=auto";
  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    // ajustar tamaño según respuesta; usar 12k para seguridad
    StaticJsonDocument<12288> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      // current weather
      if (doc.containsKey("current_weather")) {
        internetTempC = doc["current_weather"]["temperature"].as<float>();
        internetWind = doc["current_weather"]["windspeed"].as<float>();
        internetWeatherCode = doc["current_weather"]["weathercode"].as<int>();
      }
      // hourly precipitation arrays
      if (doc.containsKey("hourly") && doc["hourly"].is<JsonObject>()) {
        JsonArray times = doc["hourly"]["time"].as<JsonArray>();
        JsonArray prec = doc["hourly"]["precipitation"].as<JsonArray>();
        JsonArray precProb = doc["hourly"]["precipitation_probability"].as<JsonArray>();

        // construir string hora actual en formato ISO hora: "YYYY-MM-DDTHH:00"
        struct tm tmnow;
        if (getLocalTime(&tmnow)) {
          char curHour[32];
          strftime(curHour, sizeof(curHour), "%Y-%m-%dT%H:00", &tmnow);
          // buscar índice
          int idx = -1;
          for (size_t i = 0; i < times.size(); ++i) {
            const char* t = times[i];
            if (t && strcmp(t, curHour) == 0) { idx = i; break; }
          }
          // si no encontrado, intentar hora anterior o siguiente (robustez)
          if (idx < 0) {
            // fall back: buscar hour prefix "YYYY-MM-DDTHH"
            char curHourPrefix[32];
            strftime(curHourPrefix, sizeof(curHourPrefix), "%Y-%m-%dT%H", &tmnow);
            for (size_t i = 0; i < times.size(); ++i) {
              const char* t = times[i];
              if (t && strncmp(t, curHourPrefix, strlen(curHourPrefix)) == 0) { idx = i; break; }
            }
          }
          if (idx >= 0) {
            internetPrecipMm = prec.size() > (size_t)idx ? prec[idx].as<float>() : 0.0f;
            internetPrecipProb = precProb.size() > (size_t)idx ? precProb[idx].as<int>() : 0;
            internetUpdated = true;
            // crear descripción corta
            internetDesc = String("P:") + String(internetPrecipMm,1) + "mm " + String(internetPrecipProb) + "%";
          } else {
            // no se pudo localizar hora; marcar actualizado con valores por defecto
            internetPrecipMm = 0.0;
            internetPrecipProb = 0;
            internetUpdated = true;
            internetDesc = "P: -";
          }
        } else {
          // sin hora local, asignar pero no intentar indexado
          internetPrecipMm = 0.0;
          internetPrecipProb = 0;
          internetUpdated = true;
          internetDesc = "P: -";
        }
      }
    }
  }
  http.end();
}

// helper loop con intervalo
inline void internet_open_meteo_loop(double lat, double lon) {
  static unsigned long last = 0;
  const unsigned long interval = 10 * 60 * 1000UL; // cada 10 min
  if (millis() - last > interval) {
    last = millis();
    fetch_open_meteo(lat, lon);
  }
}