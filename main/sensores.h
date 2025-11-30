#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHT_PIN 33
#define DHT_TYPE DHT11

extern DHT dht; 
extern String fraseDia;
extern unsigned long lastFraseUpdate;
extern const unsigned long FRASE_UPDATE_INTERVAL;
extern float temperatura;
extern float humedad;
extern unsigned long lastDHTRead;
extern const unsigned long DHT_READ_INTERVAL;

inline void leerDHT11() {
  // Leer cada 2 segundos o si nunca se ha leído
  if(lastDHTRead == 0 || millis() - lastDHTRead >= DHT_READ_INTERVAL){
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // DEBUG: Imprimir en consola para ver qué pasa
    if (isnan(t) || isnan(h)) {
        Serial.println("Fallo lectura DHT!");
    } else {
        temperatura = t;
        humedad = h;
    }
    
    lastDHTRead = millis();
  }
}
inline void obtenerFraseDia() {
  if(millis()-lastFraseUpdate<FRASE_UPDATE_INTERVAL && fraseDia.length()>0) return;
  
  lastFraseUpdate = millis();
  HTTPClient http;
  http.begin("https://api.quotable.io/random?maxLength=60");
  
  int code = http.GET();
  if(code==200){
    String payload = http.getString();
    DynamicJsonDocument doc(1024); 
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) fraseDia = doc["content"].as<String>();
    else fraseDia = "Error JSON";
  } else {

     if(fraseDia.length() == 0) fraseDia = "Sin conexión";
  }
  http.end();
}