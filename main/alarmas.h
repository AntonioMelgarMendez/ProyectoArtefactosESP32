#pragma once
#include <TFT_eSPI.h>
#include <time.h>
#include <math.h> // Necesario para sin()
#include "utils.h"
#include "joystick.h"

// Necesario para emitir sonido de alarma
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// --- DEFINICIÓN DE PINES NECESARIA ---
#ifndef PIN_AMP_ENABLE
#define PIN_AMP_ENABLE 12
#endif

extern TFT_eSPI tft;
extern bool barraActiva; 
extern int navIndex;
extern AudioOutputI2S *out; 

struct Alarma {
  int hora;
  int minuto;
  bool activa;
};

extern Alarma alarmas[5];
extern int numAlarmas;
extern int alarmaSeleccionada;
extern bool mostrarModalAlarma;
extern bool modoAgregarAlarma;
extern int nuevaHora, nuevoMinuto;
extern int menuAlarmaIndex;

// Timers locales
unsigned long lastAlarmaNavTime = 0;
unsigned long lastAlarmaBtnTime = 0;

// ---------------------------------------------------------
// 1. FUNCIONES AUXILIARES DE SONIDO (ARMÓNICO)
// ---------------------------------------------------------

// Limpia el buffer de audio
void silenciarAudio() {
  if (out) {
     int16_t silence[2] = {0, 0};
     for (int i=0; i<512; i++) out->ConsumeSample(silence);
  }
}

// Genera una nota suave usando onda senoidal
// Retorna true si se presionó un botón para cancelar
bool tocarNotaSuave(int freq, int durationMs) {
  if (!out) return false;
  
  int sampleRate = 44100;
  int numSamples = (sampleRate * durationMs) / 1000;
  int16_t sample[2];
  int16_t volumen = 10000; // Volumen agradable

  for (int i = 0; i < numSamples; i++) {
    double t = (double)i / sampleRate;
    int16_t val = (int16_t)(volumen * sin(2.0 * PI * freq * t));
    
    // Fade Out al final para evitar "clic"
    if (i > numSamples - 1000) {
       val = val * (numSamples - i) / 1000;
    }

    sample[0] = val;
    sample[1] = val;
    out->ConsumeSample(sample);

    // Revisar botones frecuentemente
    if (i % 500 == 0) {
       if (leerBoton(BTN_1) || leerBoton(BTN_2)) return true; 
    }
  }
  return false;
}

// ---------------------------------------------------------
// 2. DIBUJO DE INTERFAZ (TU CÓDIGO ORIGINAL)
// ---------------------------------------------------------

void dibujarFilaAlarma(int index, bool selected) {
  int x = 60; int y = 60 + index * 40; 
  tft.startWrite();
  uint16_t bg = selected ? TFT_ORANGE : TFT_BLACK;
  uint16_t fg = selected ? TFT_BLACK : TFT_WHITE;
  tft.fillRoundRect(x - 10, y - 5, 380, 32, 6, bg);
  tft.setTextSize(2); tft.setTextColor(fg, bg); tft.setCursor(x, y);
  tft.printf("%02d:%02d", alarmas[index].hora, alarmas[index].minuto);
  tft.setCursor(x + 250, y);
  if(alarmas[index].activa) { tft.setTextColor(selected ? TFT_BLACK : TFT_GREEN, bg); tft.print("ON"); } 
  else { tft.setTextColor(selected ? TFT_BLACK : TFT_RED, bg); tft.print("OFF"); }
  tft.endWrite();
}

void dibujarBotonAgregar(bool selected) {
  int x = 60; int y = 60 + numAlarmas * 40;
  tft.startWrite();
  uint16_t bg = selected ? TFT_CYAN : TFT_DARKGREY;
  uint16_t fg = selected ? TFT_BLACK : TFT_WHITE;
  tft.fillRoundRect(x - 10, y - 5, 380, 32, 6, bg);
  tft.setTextSize(2); tft.setTextColor(fg, bg);
  String texto = "+ NUEVA ALARMA";
  int textW = tft.textWidth(texto);
  tft.setCursor(x - 10 + (380 - textW)/2, y + 8); 
  tft.print(texto);
  tft.endWrite();
}

void dibujarPantallaAlarmas(int navIndex); 

void dibujarModalAgregarAlarma(bool editandoHora, bool forceRedraw = false) {
  const int w = 260; const int h = 160; const int x = (480 - w) / 2; const int y = (320 - h) / 2;
  tft.startWrite();
  if (forceRedraw) {
    tft.fillRoundRect(x+5, y+5, w, h, 10, TFT_DARKGREY); 
    tft.fillRoundRect(x, y, w, h, 10, TFT_BLUE);         
    tft.drawRoundRect(x, y, w, h, 10, TFT_WHITE);        
    tft.setTextColor(TFT_WHITE, TFT_BLUE); tft.setTextSize(2);
    tft.drawCentreString("CONFIGURAR HORA", 240, y + 15, 2);
    tft.setTextSize(1); tft.setCursor(x + 20, y + 125); tft.print("[BTN1] Guardar");
    tft.setCursor(x + 140, y + 125); tft.print("[BTN2] Cancelar");
  }
  int boxW = 60; int boxH = 50; int gap = 10; int startX = x + (w - (boxW*2 + gap))/2; int startY = y + 50;
  uint16_t colorH = editandoHora ? TFT_YELLOW : TFT_NAVY; uint16_t textH = editandoHora ? TFT_BLACK : TFT_WHITE;
  tft.fillRoundRect(startX, startY, boxW, boxH, 8, colorH); tft.setTextColor(textH, colorH); tft.setTextSize(3); 
  int offH = (nuevaHora < 10) ? 20 : 10; tft.setCursor(startX + offH, startY + 15); tft.print(nuevaHora);
  tft.setTextColor(TFT_WHITE, TFT_BLUE); tft.setCursor(startX + boxW - 2, startY + 15); tft.print(":");
  int minX = startX + boxW + gap; uint16_t colorM = !editandoHora ? TFT_YELLOW : TFT_NAVY; uint16_t textM = !editandoHora ? TFT_BLACK : TFT_WHITE;
  tft.fillRoundRect(minX, startY, boxW, boxH, 8, colorM); tft.setTextColor(textM, colorM);
  int offM = (nuevoMinuto < 10) ? 20 : 10; tft.setCursor(minX + offM, startY + 15);
  if(nuevoMinuto < 10) tft.print("0"); tft.setCursor(minX + offM + (nuevoMinuto<10?18:15), startY + 15); tft.print(nuevoMinuto);
  tft.endWrite();
}

// ---------------------------------------------------------
// 3. POPUP DE ALARMA (CON NUEVO SONIDO)
// ---------------------------------------------------------

void sonarAlarmaBloqueante() {
  // Activar amplificador
  digitalWrite(PIN_AMP_ENABLE, HIGH);
  if(out) out->SetGain(1.0); 

  // Dibujar Popup
  tft.startWrite();
  tft.fillScreen(TFT_RED);
  tft.fillRoundRect(40, 80, 400, 160, 20, TFT_WHITE);
  tft.drawRoundRect(40, 80, 400, 160, 20, TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_WHITE); tft.setTextSize(3);
  tft.drawCentreString("! ALARMA !", 240, 110, 4);
  tft.setTextColor(TFT_BLACK, TFT_WHITE); tft.setTextSize(2);
  tft.drawCentreString("Presiona cualquier", 240, 160, 2);
  tft.drawCentreString("boton para detener", 240, 190, 2);
  tft.endWrite();

  bool cancelado = false;

  while(!cancelado) {
      // Reproducir Melodía Suave (Do-Mi-Sol)
      // Si tocan un botón durante la nota, retorna true y salimos
      if (tocarNotaSuave(523, 300)) { cancelado = true; break; } // Do
      delay(50);
      if (tocarNotaSuave(659, 300)) { cancelado = true; break; } // Mi
      delay(50);
      if (tocarNotaSuave(784, 600)) { cancelado = true; break; } // Sol

      // Espera silenciosa entre repeticiones (1 segundo)
      unsigned long wait = millis();
      while(millis() - wait < 1000) {
          if (leerBoton(BTN_1) || leerBoton(BTN_2)) {
              cancelado = true;
              break;
          }
          delay(10);
      }
  }
  
  // Salida limpia
  silenciarAudio();
  digitalWrite(PIN_AMP_ENABLE, LOW); 
  if(out) out->SetGain(0.3); 
  
  mostrarModalAlarma = false;
  if(alarmaSeleccionada >= 0) alarmas[alarmaSeleccionada].activa = false;
  dibujarPantallaAlarmas(navIndex); 
}

// ---------------------------------------------------------
// 4. LÓGICA PRINCIPAL
// ---------------------------------------------------------

void dibujarPantallaAlarmas(int navIndex) {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex, true); 
  for (int i = 0; i < numAlarmas; i++) dibujarFilaAlarma(i, i == 0);
  dibujarBotonAgregar(numAlarmas == 0);
}

inline void actualizarPantallaAlarmas() {
  unsigned long now = millis();
  static bool editandoHora = true;
  static int lastIndex = -1;
  
  static int lastH = -1; static int lastM = -1;
  static bool lastEditMode = false; static bool modalAbiertoPrevio = false;
  static int ultimoMinutoSonado = -1;

  // === CHECK ALARMA ===
  struct tm t; 
  if(getLocalTime(&t)) {
      if (t.tm_min != ultimoMinutoSonado) ultimoMinutoSonado = -1; 

      for (int i = 0; i < numAlarmas; i++) {
        if (alarmas[i].activa && t.tm_hour == alarmas[i].hora && t.tm_min == alarmas[i].minuto && t.tm_sec == 0 && ultimoMinutoSonado != t.tm_min) {
          alarmaSeleccionada = i;
          sonarAlarmaBloqueante(); 
          ultimoMinutoSonado = t.tm_min; 
          return; 
        }
      }
  }

  // === MODO EDICIÓN ===
  if (modoAgregarAlarma) {
      bool firstOpen = !modalAbiertoPrevio;
      if (firstOpen || lastH != nuevaHora || lastM != nuevoMinuto || lastEditMode != editandoHora) {
          dibujarModalAgregarAlarma(editandoHora, firstOpen); 
          lastH = nuevaHora; lastM = nuevoMinuto; lastEditMode = editandoHora;
      }
      modalAbiertoPrevio = true;

      if (now - lastAlarmaNavTime > 150) {
          DPadDir dir = detectarDireccionJoystick();
          if (dir == DPAD_LEFT) editandoHora = true;
          if (dir == DPAD_RIGHT) editandoHora = false;
          if (dir == DPAD_UP) { if(editandoHora) nuevaHora = (nuevaHora + 1) % 24; else nuevoMinuto = (nuevoMinuto + 1) % 60; lastAlarmaNavTime = now; }
          if (dir == DPAD_DOWN) { if(editandoHora) nuevaHora = (nuevaHora - 1 + 24) % 24; else nuevoMinuto = (nuevoMinuto - 1 + 60) % 60; lastAlarmaNavTime = now; }
      }

      if (now - lastAlarmaBtnTime > 300) {
          if (leerBoton(BTN_1)) { 
              if (numAlarmas < 5) { alarmas[numAlarmas].hora = nuevaHora; alarmas[numAlarmas].minuto = nuevoMinuto; alarmas[numAlarmas].activa = true; numAlarmas++; }
              modoAgregarAlarma = false; modalAbiertoPrevio = false; dibujarPantallaAlarmas(navIndex); 
          }
          if (leerBoton(BTN_2)) { modoAgregarAlarma = false; modalAbiertoPrevio = false; dibujarPantallaAlarmas(navIndex); }
          lastAlarmaBtnTime = now;
      }
      return; 
  }

  // === MODO LISTA ===
  if (barraActiva) return; 

  if (now - lastAlarmaNavTime > 150) {
      DPadDir dir = detectarDireccionJoystick();
      if (dir == DPAD_DOWN) { if (menuAlarmaIndex < numAlarmas) { menuAlarmaIndex++; lastAlarmaNavTime = now; } } 
      else if (dir == DPAD_UP) { if (menuAlarmaIndex > 0) { menuAlarmaIndex--; lastAlarmaNavTime = now; } else { barraActiva = true; return; } }
  }

  if (menuAlarmaIndex != lastIndex) {
      if (lastIndex == numAlarmas) dibujarBotonAgregar(false); else if (lastIndex >= 0) dibujarFilaAlarma(lastIndex, false);
      if (menuAlarmaIndex == numAlarmas) dibujarBotonAgregar(true); else dibujarFilaAlarma(menuAlarmaIndex, true);
      lastIndex = menuAlarmaIndex;
  }

  if (now - lastAlarmaBtnTime > 300 && leerBoton(BTN_1)) {
      if (menuAlarmaIndex == numAlarmas) {
          modoAgregarAlarma = true; nuevaHora = 8; nuevoMinuto = 0; editandoHora = true; tft.fillRect(20, 20, 440, 280, TFT_BLACK); 
      } else {
          alarmas[menuAlarmaIndex].activa = !alarmas[menuAlarmaIndex].activa; dibujarFilaAlarma(menuAlarmaIndex, true); 
      }
      lastAlarmaBtnTime = now;
  }
}