#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern bool barraActiva; 

// Añadimos 'bool forzar' para cuando entras a la pantalla por primera vez
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

  // Si no forzamos y nada ha cambiado, SALIR RÁPIDO para no gastar CPU
  if (!forzar && lastSelected == seleccionado && lastBarraActiva == barraActiva) return;

  uint16_t bg = barraActiva ? TFT_DARKGREY : TFT_BLACK;

  tft.startWrite();

  // 1. DIBUJADO COMPLETO (Solo al entrar a la pantalla o cambiar estado activo/inactivo)
  if (forzar || lastBarraActiva != barraActiva) {
    tft.fillRect(0, 0, tft.width(), 40, bg);
    // Forzamos que se redibujen todos los textos
    lastSelected = -1; 
  }

  // 2. ACTUALIZACIÓN PARCIAL (Dirty Rectangles)
  for (int i = 0; i < 3; ++i) {
    // Solo tocamos la pantalla si este elemento cambió (era el seleccionado o es el nuevo)
    // O si estamos forzando un redibujado completo (-1)
    if (i == seleccionado || i == lastSelected || lastSelected == -1) {
        
        int tx = baseX + i * gap;
        
        // BORRADO SELECTIVO: Si no estamos forzando todo, borramos SOLO este botón
        // Esto evita borrar 480 pixeles de ancho, lo que mataba el audio
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