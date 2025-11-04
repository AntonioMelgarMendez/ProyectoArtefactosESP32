#pragma once
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern bool barraActiva; // indica si la barra de navegación tiene el foco

inline void dibujarBarraNavegacion(int seleccionado) {
  const char* titulos[] = {"Dashboard", "Alarmas", "Musica"};
  const int baseX = 30;
  const int gap = 150;
  const int boxW = 120;
  const int boxH = 28;
  const int boxY = 6;
  const int arrowY = 4;
  const int arrowH = 8;
  const int arrowHalf = 6;

  static int lastSelected = -1;
  static bool lastBarraActiva = false;

  // Si no hay cambios, salir rápido
  if (lastSelected == seleccionado && lastBarraActiva == barraActiva) return;

  uint16_t bg = barraActiva ? TFT_DARKGREY : TFT_BLACK;

  tft.startWrite();
  // Redibujar toda la barra para evitar artefactos al limpiar parcialmente
  tft.fillRect(0, 0, tft.width(), 40, bg);

  for (int i = 0; i < 3; ++i) {
    int tx = baseX + i * gap;
    if (barraActiva && i == seleccionado) {
      // cuadro de selección naranja + texto negro
      tft.fillRoundRect(tx - 8, boxY, boxW, boxH, 6, TFT_ORANGE);
      tft.setTextColor(TFT_BLACK);
    } else {
      // color normal (naranja si seleccionado pero barra inactiva, blanco en general)
      tft.setTextColor((i == seleccionado && !barraActiva) ? TFT_ORANGE : TFT_WHITE);
    }
    tft.setTextSize(2);
    tft.setCursor(tx, 10);
    tft.print(titulos[i]);
  }

  // flecha cuando está activa
  if (barraActiva && seleccionado >= 0) {
    int centerX = baseX + seleccionado * gap + 30;
    tft.fillTriangle(centerX, arrowY, centerX - arrowHalf, arrowY + arrowH, centerX + arrowHalf, arrowY + arrowH, TFT_ORANGE);
  }

  tft.endWrite();

  lastSelected = seleccionado;
  lastBarraActiva = barraActiva;
}