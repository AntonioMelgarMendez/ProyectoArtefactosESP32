#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h> 
#include "DHT.h"

#define DHT_PIN 33
#define DHT_TYPE DHT11

extern DHT dht; 
extern String fraseDia;
extern unsigned long lastFraseUpdate;
extern const unsigned long FRASE_UPDATE_INTERVAL;
extern volatile float temperatura;
extern volatile float humedad;

const char* SERVER_URL = "http://192.168.0.14:3000/api/sensores"; 

unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;

const unsigned long READ_INTERVAL = 3000;  
const unsigned long SEND_INTERVAL = 35000;  
inline void gestionarSensoresYEnvio(bool forzarEnvio = false) {
  unsigned long now = millis();

  if (now - lastReadTime > READ_INTERVAL || lastReadTime == 0) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
       Serial.println("[SENSOR] Fallo lectura (NaN). Manteniendo valor anterior.");
    } else {
       temperatura = t;
       humedad = h;
       Serial.printf("[SENSOR] Lectura: %.1f C, %.1f %%\n", temperatura, humedad);
    }
    lastReadTime = now;
  }

  if (forzarEnvio || (now - lastSendTime > SEND_INTERVAL)) {
    if (WiFi.status() == WL_CONNECTED) {
        
        HTTPClient http;
        http.setTimeout(4000); 
        
        Serial.println("[CLOUD] Conectando a backend...");
        
        if (http.begin(SERVER_URL)) {
            http.addHeader("Content-Type", "application/json");
            String jsonPayload = "{\"temperatura\": " + String(temperatura) + 
                                 ", \"humedad\": " + String(humedad) + "}";
            
            int httpCode = http.POST(jsonPayload);
            
            if (httpCode > 0) {
                Serial.printf("[CLOUD] Enviado OK. Código: %d\n", httpCode);
            } else {
                Serial.printf("[CLOUD] Error HTTP: %s\n", http.errorToString(httpCode).c_str());
            }
            http.end();
        } else {
            Serial.println("[CLOUD] No se pudo iniciar conexión HTTP");
        }
    } else {
        Serial.println("[CLOUD] WiFi desconectado. No se envió.");
    }
    lastSendTime = now;
  }
}

inline void obtenerFraseDia() {
  if(millis()-lastFraseUpdate<FRASE_UPDATE_INTERVAL && fraseDia.length()>0) return;
  lastFraseUpdate = millis();
  
  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(3000); 
  http.begin("https://api.quotable.io/random?maxLength=60");
  int code = http.GET();
  if(code==200){
    String payload = http.getString();
    DynamicJsonDocument doc(1024); 
    deserializeJson(doc, payload);
    if(doc.containsKey("content")) {
        fraseDia = doc["content"].as<String>();
    }
  } 
  http.end();
}