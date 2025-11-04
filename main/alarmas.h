#pragma once
#include <TFT_eSPI.h>
#include "utils.h"
#include "joystick.h"

extern TFT_eSPI tft;
extern bool barraActiva; 

// Definición del tipo Alarma
struct Alarma {
  int hora;
  int minuto;
  bool activa;
};

// Variables externas (definidas en main.ino)
extern Alarma alarmas[5];
extern int numAlarmas;
extern int alarmaSeleccionada;
extern bool mostrarModalAlarma;
extern bool modoAgregarAlarma;
extern int nuevaHora, nuevoMinuto;
extern int menuAlarmaIndex;

// dibuja una fila de alarma (index dentro del listado)
inline void dibujarFilaAlarma(int index, bool selected) {
  int x = 60;
  int y = 60 + index * 30;
  // fondo para selección
  if (selected) tft.fillRect(x - 6, y - 2, 360, 22, TFT_DARKGREY);
  else tft.fillRect(x - 6, y - 2, 360, 22, TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(selected ? TFT_CYAN : TFT_WHITE);
  tft.setCursor(x, y);
  tft.printf("%d) %02d:%02d %s", index + 1, alarmas[index].hora, alarmas[index].minuto, alarmas[index].activa ? "ON" : "OFF");
}

// dibuja la línea "+ Agregar"
inline void dibujarFilaAgregar(bool selected) {
  int x = 60;
  int y = 60 + numAlarmas * 30;
  if (selected) tft.fillRect(x - 6, y - 2, 360, 22, TFT_DARKGREY);
  else tft.fillRect(x - 6, y - 2, 360, 22, TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(selected ? TFT_ORANGE : TFT_WHITE);
  tft.setCursor(x, y);
  tft.print("+ Agregar nueva alarma");
}

inline void dibujarPantallaAlarmas(int navIndex) {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex);

  // dibujar todo el listado (se llamará una sola vez al entrar)
  for (int i = 0; i < numAlarmas; i++) dibujarFilaAlarma(i, i == menuAlarmaIndex);
  dibujarFilaAgregar(menuAlarmaIndex == numAlarmas);

  tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
  tft.setCursor(30, 250); tft.print("Arriba/Abajo: Navegar  BTN1: Seleccionar  BTN2: Cancelar");
}

// Modal centrado: dibuja contenido según valores actuales
inline void dibujarModalAgregarAlarma(bool editHora) {
  const int modalW = 320, modalH = 160;
  int mx = (tft.width() - modalW) / 2;
  int my = (tft.height() - modalH) / 2;

  // fondo y borde
  tft.fillRoundRect(mx, my, modalW, modalH, 12, TFT_YELLOW);
  tft.drawRoundRect(mx, my, modalW, modalH, 12, TFT_RED);

  tft.setTextSize(2); tft.setTextColor(TFT_RED);
  tft.setCursor(mx + 24, my + 12); tft.print("Nueva alarma");

  // hora:min con indicador de selección
  tft.setTextSize(3);
  // limpiar área del tiempo
  tft.fillRect(mx + 24, my + 44, modalW - 48, 60, TFT_YELLOW);

  // pintar hora y minuto
  int hx = mx + 40;
  int hy = my + 56;
  // hora
  if (editHora) {
    tft.fillRoundRect(hx - 6, hy - 6, 88, 40, 6, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.setTextColor(TFT_BLACK);
  }
  tft.setCursor(hx, hy);
  tft.printf("%02d", nuevaHora);

  // separador
  tft.setTextColor(TFT_BLACK); tft.setCursor(hx + 48, hy); tft.print(":");

  // minuto
  int mxpos = hx + 64;
  if (!editHora) {
    tft.fillRoundRect(mxpos - 6, hy - 6, 88, 40, 6, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.setTextColor(TFT_BLACK);
  }
  tft.setCursor(mxpos, hy);
  tft.printf("%02d", nuevoMinuto);

  // instrucciones
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(mx + 24, my + modalH - 36);
  tft.print("Izq/Der: Seleccionar  Arriba/Abajo: Cambiar");
  tft.setCursor(mx + 24, my + modalH - 20);
  tft.print("BTN1: Guardar   BTN2: Cancelar");
}

// Actualización eficiente: sólo redibuja filas cambiadas y modal cuando corresponde
inline void actualizarPantallaAlarmas() {
  static bool listDrawn = false;
  static int lastMenuIndex = -1;
  static int lastNumAlarmas = -1;
  static bool lastActive[5] = {false,false,false,false,false};
  static bool modalDrawn = false;
  static bool editHora = true;
  static DPadDir lastDir = DPAD_NONE;

  // si no estamos en modal, gestionar listado
  if (!modoAgregarAlarma) {
    modalDrawn = false;
    // (re)dibuja lista completa si entramos por primera vez o cambió el número de alarmas
    if (!listDrawn || lastNumAlarmas != numAlarmas) {
      // redibujar contenedor
      tft.fillRect(0, 48, tft.width(), tft.height()-48, TFT_BLACK); // limpia la zona de contenido
      for (int i = 0; i < numAlarmas; i++) {
        dibujarFilaAlarma(i, i == menuAlarmaIndex);
        lastActive[i] = alarmas[i].activa;
      }
      dibujarFilaAgregar(menuAlarmaIndex == numAlarmas);
      lastNumAlarmas = numAlarmas;
      listDrawn = true;
      lastMenuIndex = menuAlarmaIndex;
      return;
    }

    // actualizar filas cuya activacion cambió
    for (int i = 0; i < numAlarmas; i++) {
      if (alarmas[i].activa != lastActive[i]) {
        dibujarFilaAlarma(i, i == menuAlarmaIndex);
        lastActive[i] = alarmas[i].activa;
      }
    }

    // actualizar selección si cambió
    if (menuAlarmaIndex != lastMenuIndex) {
      // repintar anterior fila sin selección
      if (lastMenuIndex >= 0 && lastMenuIndex < numAlarmas) dibujarFilaAlarma(lastMenuIndex, false);
      else if (lastMenuIndex == numAlarmas) dibujarFilaAgregar(false);

      // pintar nueva fila seleccionada
      if (menuAlarmaIndex >= 0 && menuAlarmaIndex < numAlarmas) dibujarFilaAlarma(menuAlarmaIndex, true);
      else if (menuAlarmaIndex == numAlarmas) dibujarFilaAgregar(true);

      lastMenuIndex = menuAlarmaIndex;
      delay(120);
    }

    // navegación
    DPadDir dir = detectarDireccionJoystick();
     if (barraActiva) dir = DPAD_NONE;
    if (dir == DPAD_UP && lastDir != DPAD_UP) {
      menuAlarmaIndex = (menuAlarmaIndex - 1 + numAlarmas + 1) % (numAlarmas + 1);
      delay(150);
    } else if (dir == DPAD_DOWN && lastDir != DPAD_DOWN) {
      menuAlarmaIndex = (menuAlarmaIndex + 1) % (numAlarmas + 1);
      delay(150);
    }

    // seleccionar / toggle / entrar modal
    if (leerBoton(BTN_1)) {
      if (menuAlarmaIndex < numAlarmas) {
        alarmas[menuAlarmaIndex].activa = !alarmas[menuAlarmaIndex].activa;
        dibujarFilaAlarma(menuAlarmaIndex, menuAlarmaIndex == lastMenuIndex);
      } else {
        // abrir modal
        modoAgregarAlarma = true;
        nuevaHora = 7;
        nuevoMinuto = 0;
        modalDrawn = false;
        editHora = true;
      }
      delay(200);
    }

    lastDir = dir;
  } else {
    // MODO MODAL: dibujar modal una sola vez y actualizar campos sólo cuando cambian
    DPadDir dir = detectarDireccionJoystick();
    if (!modalDrawn) {
      // limpiar área detrás del modal (suaviza)
      tft.fillRect(40, 70, tft.width()-80, tft.height()-140, TFT_BLACK);
      dibujarModalAgregarAlarma(editHora);
      modalDrawn = true;
    } else {
      // detectar navegación horizontal para cambiar campo seleccionado
      if (dir == DPAD_LEFT && lastDir != DPAD_LEFT) { editHora = true; dibujarModalAgregarAlarma(editHora); delay(120); }
      else if (dir == DPAD_RIGHT && lastDir != DPAD_RIGHT) { editHora = false; dibujarModalAgregarAlarma(editHora); delay(120); }

      // up/down cambia valor del campo seleccionado
      if (dir == DPAD_UP && lastDir != DPAD_UP) {
        if (editHora) nuevaHora = (nuevaHora + 1) % 24;
        else nuevoMinuto = (nuevoMinuto + 1) % 60;
        dibujarModalAgregarAlarma(editHora);
        delay(120);
      } else if (dir == DPAD_DOWN && lastDir != DPAD_DOWN) {
        if (editHora) nuevaHora = (nuevaHora - 1 + 24) % 24;
        else nuevoMinuto = (nuevoMinuto - 1 + 60) % 60;
        dibujarModalAgregarAlarma(editHora);
        delay(120);
      }

      // BTN1: guardar
      if (leerBoton(BTN_1)) {
        if (numAlarmas < 5) {
          alarmas[numAlarmas].hora = nuevaHora;
          alarmas[numAlarmas].minuto = nuevoMinuto;
          alarmas[numAlarmas].activa = true;
          numAlarmas++;
        }
        modoAgregarAlarma = false;
        // forzar re-draw del listado en la próxima iteración
        listDrawn = false;
        modalDrawn = false;
        delay(300);
      }

      // BTN2: cancelar
      if (leerBoton(BTN_2)) {
        modoAgregarAlarma = false;
        listDrawn = false;
        modalDrawn = false;
        delay(300);
      }

      lastDir = dir;
    }
  }

  // comprobación de alarmas activas para mostrar modal de alarma si corresponde
  struct tm t; getLocalTime(&t);
  for (int i = 0; i < numAlarmas; i++) {
    if (alarmas[i].activa && t.tm_hour == alarmas[i].hora && t.tm_min == alarmas[i].minuto && !mostrarModalAlarma) {
      mostrarModalAlarma = true;
      alarmaSeleccionada = i;
    }
  }

  if (mostrarModalAlarma) {
    tft.fillRoundRect(80, 100, 320, 100, 12, TFT_YELLOW);
    tft.setTextColor(TFT_RED); tft.setTextSize(3);
    tft.setCursor(120, 130); tft.print("¡ALARMA!");
    tft.setTextColor(TFT_BLACK); tft.setTextSize(2);
    tft.setCursor(120, 170); tft.print("Presiona cualquier boton");
    if (leerBoton(BTN_1) || leerBoton(BTN_2)) {
      mostrarModalAlarma = false;
      alarmas[alarmaSeleccionada].activa = false;
      // forzar redraw de la fila apagada
      if (alarmaSeleccionada >= 0 && alarmaSeleccionada < numAlarmas) {
        dibujarFilaAlarma(alarmaSeleccionada, menuAlarmaIndex == alarmaSeleccionada);
      }
      delay(500);
    }
  }
}