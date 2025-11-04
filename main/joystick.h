#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Pines del joystick y botones
#define JOY_X 34
#define JOY_Y 32
#define JOY_SW 25 
#define BTN_1  17
#define BTN_2  16

extern TFT_eSPI tft; // usado por UI en calibración (definido en main.ino)

// Tipos compartidos
enum DPadDir { DPAD_NONE, DPAD_LEFT, DPAD_RIGHT, DPAD_UP, DPAD_DOWN };
enum Pantalla { DASHBOARD, ALARMAS, MUSICA };

struct JoystickCal {
  int x_min=0, x_max=4095, x_center=2048;
  int y_min=0, y_max=4095, y_center=2048;
  int deadzone=8;
  bool x_invertido=false;
  bool y_invertido=false;
  bool done=false;
};
extern JoystickCal cal;

enum CalState { INIT, CENTER, LEFT, RIGHT, UP, DOWN, COMPLETE };
extern CalState cal_state;

// Funciones usadas por el sketch
inline bool leerBoton(int pin) {
  static unsigned long lastPress = 0;
  bool currentState = digitalRead(pin) == LOW;
  if (currentState && millis() - lastPress > 500) {
    lastPress = millis();
    return true;
  }
  return false;
}

inline int leerPromedio(int pin, int muestras = 20) {
  long suma = 0;
  for (int i = 0; i < muestras; i++) {
    suma += analogRead(pin);
    delay(2);
  }
  return suma / muestras;
}

DPadDir detectarDireccionJoystick();

inline DPadDir detectarDireccionJoystick() {
  if (!cal.done) return DPAD_NONE;
  int x_val = leerPromedio(JOY_X, 10);
  int y_val = leerPromedio(JOY_Y, 10);
  int cx = cal.x_center, cy = cal.y_center;
  int thr = 150;
  if (x_val < cx - thr) return DPAD_LEFT;
  if (x_val > cx + thr) return DPAD_RIGHT;
  if (y_val < cy - thr) return DPAD_UP;
  if (y_val > cy + thr) return DPAD_DOWN;
  return DPAD_NONE;
}

// Calibración interactiva (usa extern cal y cal_state)
inline void procesoCalibracion() {
  int x_val = leerPromedio(JOY_X,15);
  int y_val = leerPromedio(JOY_Y,15);
  tft.fillRect(30,180,420,30,TFT_BLACK);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(40,185);
  tft.printf("X:%d Y:%d",x_val,y_val);

  if(leerBoton(BTN_1)){
    switch(cal_state){
      case INIT:
        cal_state=CENTER;
        tft.fillScreen(TFT_NAVY);
        tft.setTextColor(TFT_CYAN); tft.setTextSize(3);
        tft.setCursor(30,140); tft.print("Deja el joystick CENTRADO");
        delay(1000);
        break;
      case CENTER:
        cal.x_center = leerPromedio(JOY_X,50);
        cal.y_center = leerPromedio(JOY_Y,50);
        cal_state=LEFT;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve IZQUIERDA");
        delay(1000);
        break;
      case LEFT:
        cal.x_min = leerPromedio(JOY_X);
        cal_state=RIGHT;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve DERECHA");
        delay(1000);
        break;
      case RIGHT:
        cal.x_max = leerPromedio(JOY_X);
        cal_state=UP;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve ARRIBA");
        delay(1000);
        break;
      case UP:
        cal.y_min = leerPromedio(JOY_Y);
        cal_state=DOWN;
        tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve ABAJO");
        delay(1000);
        break;
      case DOWN:
        cal.y_max = leerPromedio(JOY_Y);
        cal_state=COMPLETE;
        cal.x_invertido = cal.x_min > cal.x_max;
        cal.y_invertido = cal.y_min > cal.y_max;
        cal.done = true;
        break;
      default: break;
    }
  }
}