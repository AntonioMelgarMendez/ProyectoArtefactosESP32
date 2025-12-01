#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

#define JOY_X 34
#define JOY_Y 32

#define BTN_1  17
#define BTN_2  16

extern TFT_eSPI tft; 

enum DPadDir { DPAD_NONE, DPAD_LEFT, DPAD_RIGHT, DPAD_UP, DPAD_DOWN };
enum Pantalla { DASHBOARD, ALARMAS, MUSICA };

struct JoystickCal {
  int x_min = 235; 
  int x_max = 4095; 
  int x_center = 3314; 
  
  int y_min = 4095; 
  int y_max = 268; 
  int y_center = 3153; 
  int deadzone = 600; 
  
  bool x_invertido = false;
  bool y_invertido = true;

  bool done = true; 
};

extern JoystickCal cal;

enum CalState { INIT, CENTER, LEFT, RIGHT, UP, DOWN, COMPLETE };
extern CalState cal_state;
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

inline DPadDir detectarDireccionJoystick() {
  int raw_x = leerPromedio(JOY_X, 10);
  int raw_y = leerPromedio(JOY_Y, 10);
  
  int diff_x = raw_x - cal.x_center;
  int diff_y = raw_y - cal.y_center;
  
  int abs_x = abs(diff_x);
  int abs_y = abs(diff_y);
  
  if (abs_x < cal.deadzone && abs_y < cal.deadzone) return DPAD_NONE;
  
  int margen_seguridad = 300; 

  if (abs_x > (abs_y + margen_seguridad)) {
    if (diff_x < 0) return cal.x_invertido ? DPAD_RIGHT : DPAD_LEFT;
    if (diff_x > 0) return cal.x_invertido ? DPAD_LEFT : DPAD_RIGHT;
  } 
  else if (abs_y > (abs_x + margen_seguridad)) {
    if (diff_y < 0) return cal.y_invertido ? DPAD_DOWN : DPAD_UP;
    if (diff_y > 0) return cal.y_invertido ? DPAD_UP : DPAD_DOWN;
  }
  return DPAD_NONE;
}

inline void procesoCalibracion() {
}