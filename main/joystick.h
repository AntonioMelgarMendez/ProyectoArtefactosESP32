#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Pines del joystick (Ejes)
#define JOY_X 34
#define JOY_Y 32

// Botones de la interfaz (Externos al stick)
#define BTN_1  17
#define BTN_2  16

extern TFT_eSPI tft; 

// Tipos compartidos
enum DPadDir { DPAD_NONE, DPAD_LEFT, DPAD_RIGHT, DPAD_UP, DPAD_DOWN };
enum Pantalla { DASHBOARD, ALARMAS, MUSICA };

struct JoystickCal {
  // AJUSTADO A TUS VALORES OBSERVADOS (Centro ~2000/1800)
  int x_min=0, x_max=4095, x_center=2000; 
  int y_min=0, y_max=4095, y_center=1800; 
  
  // Zona muerta aumentada para evitar "ruido" y activaciones fantasma
  int deadzone=200; 
  
  bool x_invertido=false;
  bool y_invertido=false;
  bool done=false; 
};
extern JoystickCal cal;

enum CalState { INIT, CENTER, LEFT, RIGHT, UP, DOWN, COMPLETE };
extern CalState cal_state;

// Funciones utilitarias
inline bool leerBoton(int pin) {
  static unsigned long lastPress = 0;
  bool currentState = digitalRead(pin) == LOW;
  if (currentState && millis() - lastPress > 500) {
    lastPress = millis();
    return true;
  }
  return false;
}

inline int leerPromedio(int pin, int muestras = 15) {
  long suma = 0;
  for (int i = 0; i < muestras; i++) {
    suma += analogRead(pin);
    delayMicroseconds(50); 
  }
  return (int)(suma / muestras);
}

// Lógica MEJORADA con Zona Muerta Amplia
inline DPadDir detectarDireccionJoystick() {
  // 1. Leer valores actuales
  int raw_x = leerPromedio(JOY_X, 10);
  int raw_y = leerPromedio(JOY_Y, 10);

  // 2. Calcular desviación
  int diff_x = raw_x - cal.x_center;
  int diff_y = raw_y - cal.y_center;

  // 3. Magnitud
  int abs_x = abs(diff_x);
  int abs_y = abs(diff_y);

  // 4. Calcular umbral de seguridad
  int umbral = 300; 
  if (cal.done) {
     int rango_x = abs(cal.x_max - cal.x_min) / 2;
     int umbral_dinamico = rango_x * 0.40; 
     if (umbral_dinamico > umbral) umbral = umbral_dinamico;
  }

  // 5. Filtrar ruido (Centro)
  if (abs_x < umbral && abs_y < umbral) return DPAD_NONE;

  // 6. Eje Dominante
  if (abs_x > abs_y) {
    if (diff_x < 0) return cal.x_invertido ? DPAD_RIGHT : DPAD_LEFT;
    if (diff_x > 0) return cal.x_invertido ? DPAD_LEFT : DPAD_RIGHT;
  } else {
    if (diff_y < 0) return cal.y_invertido ? DPAD_DOWN : DPAD_UP;
    if (diff_y > 0) return cal.y_invertido ? DPAD_UP : DPAD_DOWN;
  }

  return DPAD_NONE;
}

// Calibración interactiva
inline void procesoCalibracion() {
  int x_val = leerPromedio(JOY_X, 15);
  int y_val = leerPromedio(JOY_Y, 15);
  
  tft.fillRect(30, 180, 420, 30, TFT_BLACK);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); 
  tft.setCursor(40, 185);
  tft.printf("RAW X:%d  RAW Y:%d", x_val, y_val);

  if(leerBoton(BTN_1)){
    tft.fillRect(0, 130, 480, 50, TFT_NAVY); 
    switch(cal_state){
      case INIT:
        cal_state = CENTER;
        tft.fillScreen(TFT_NAVY);
        tft.setTextColor(TFT_CYAN); tft.setTextSize(3);
        tft.setCursor(30, 140); tft.print("Centro: NO TOCAR");
        delay(500);
        break;
      case CENTER:
        cal.x_center = leerPromedio(JOY_X, 50);
        cal.y_center = leerPromedio(JOY_Y, 50);
        cal_state = LEFT;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30, 140); tft.print("Mover IZQUIERDA");
        delay(500);
        break;
      case LEFT:
        cal.x_min = leerPromedio(JOY_X, 30);
        cal_state = RIGHT;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30, 140); tft.print("Mover DERECHA");
        delay(500);
        break;
      case RIGHT:
        cal.x_max = leerPromedio(JOY_X, 30);
        cal_state = UP;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30, 140); tft.print("Mover ARRIBA");
        delay(500);
        break;
      case UP:
        cal.y_min = leerPromedio(JOY_Y, 30);
        cal_state = DOWN;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30, 140); tft.print("Mover ABAJO");
        delay(500);
        break;
      case DOWN:
        cal.y_max = leerPromedio(JOY_Y, 30);
        cal_state = COMPLETE;
        cal.x_invertido = (cal.x_min > cal.x_max);
        cal.y_invertido = (cal.y_min > cal.y_max);
        cal.done = true;
        tft.fillScreen(TFT_BLACK);
        break;
      default: break;
    }
  }
}