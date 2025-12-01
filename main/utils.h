#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern bool barraActiva; 

inline void dibujarBarraNavegacion(int seleccionado, bool forzar = false) {
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

  if (!forzar && lastSelected == seleccionado && lastBarraActiva == barraActiva) return;

  uint16_t bg = barraActiva ? TFT_DARKGREY : TFT_BLACK;

  tft.startWrite();
  if (forzar || lastBarraActiva != barraActiva) {
    tft.fillRect(0, 0, tft.width(), 40, bg);
    lastSelected = -1; 
  }

  for (int i = 0; i < 3; ++i) {
    if (i == seleccionado || i == lastSelected || lastSelected == -1) {
        
        int tx = baseX + i * gap;
        if (!forzar && lastBarraActiva == barraActiva) {
             tft.fillRect(tx - 10, 0, boxW + 20, 40, bg);
        }

        // Dibujar caja naranja si toca
        if (barraActiva && i == seleccionado) {
          tft.fillRoundRect(tx - 8, boxY, boxW, boxH, 6, TFT_ORANGE);
          tft.setTextColor(TFT_BLACK);
        } else {
          tft.setTextColor((i == seleccionado && !barraActiva) ? TFT_ORANGE : TFT_WHITE);
        }
        
        tft.setTextSize(2);
        tft.setCursor(tx, 10);
        tft.print(titulos[i]);

        // Dibujar flechita
        if (barraActiva && i == seleccionado) {
            int centerX = baseX + seleccionado * gap + 30;
            tft.fillTriangle(centerX, arrowY, centerX - arrowHalf, arrowY + arrowH, centerX + arrowHalf, arrowY + arrowH, TFT_ORANGE);
        }
    }
  }

  tft.endWrite();

  lastSelected = seleccionado;
  lastBarraActiva = barraActiva;
}