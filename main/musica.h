// ...existing code...
#pragma once
#include <TFT_eSPI.h>
#include "utils.h"
#include "joystick.h"

extern TFT_eSPI tft;
extern int navIndex;
extern bool barraActiva; // añadir


// Estado de reproducción
enum MusicaEstado { PLAY, PAUSE };
extern MusicaEstado estadoMusica;

extern unsigned long musicaStartTime;
extern unsigned long musicaDurationSec; // duración total en segundos
extern int volumen;
extern int musicaAccionIndex;
extern String cancionActual;

// --- Pantalla música: dibuja sólo elementos estáticos (llamar una vez al entrar) ---
inline void dibujarPantallaMusica(int nav) {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(nav);

  // Portada: cuadro y nota vectorial centrada
  const int ax = 30;
  const int ay = 40;
  const int coverW = 96;
  const int coverH = 96;
  tft.fillRoundRect(ax, ay, coverW, coverH, 8, TFT_DARKGREY);
  tft.fillRoundRect(ax + 6, ay + 6, coverW - 12, coverH - 12, 6, TFT_BLACK);

  // dibujar nota vectorial centrada (evita problemas de fuentes/UTF8)
  int cx = ax + coverW / 2;
  int cy = ay + coverH / 2;
  // cabeza
  tft.fillCircle(cx - 8, cy + 6, 12, TFT_YELLOW);
  // stem
  tft.fillRect(cx + 2, cy - 22, 6, 38, TFT_YELLOW);
  // una plica simple/cola
  tft.fillTriangle(cx + 8, cy - 10, cx + 22, cy - 2, cx + 8, cy, TFT_YELLOW);

  // --- Título centrado (aseguro textSize antes de medir) ---
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  int16_t titleW = tft.textWidth(cancionActual);
  int titleX = (tft.width() - titleW) / 2;
  if (titleX < ax + coverW + 10) titleX = ax + coverW + 10;
  tft.setCursor(titleX, 60);
  tft.print(cancionActual);

  // marco progress bar
  int pbX = titleX;
  int pbY = 120;
  int pbW = tft.width() - titleX - 20;
  int pbH = 8;
  tft.drawRoundRect(pbX - 1, pbY - 1, pbW + 2, pbH + 2, 4, TFT_DARKGREY);
  // inicial fill vacío
  tft.fillRect(pbX, pbY, 0, pbH, TFT_ORANGE);
  tft.fillRect(pbX + 0, pbY, pbW, pbH, TFT_DARKGREY);

  // botones (dibujado inicial; sus textos vienen del estado actual)
  const char* acciones[] = {"<<", estadoMusica == PLAY ? "||" : ">", ">>", "Vol-", "Vol+"};
  for (int i = 0; i < 5; i++) {
    int x = 60 + i * 80;
    int y = 220;
    tft.fillRoundRect(x, y, 70, 40, 8, musicaAccionIndex == i ? TFT_ORANGE : TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(3);
    int tw = tft.textWidth(acciones[i]);
    tft.setCursor(x + (70 - tw) / 2, y + 6);
    tft.print(acciones[i]);
  }

  // volumen
  tft.setTextSize(2); tft.setTextColor(TFT_CYAN);
  tft.setCursor(200, 270); tft.printf("Vol: %d", volumen);
}

// --- Actualiza sólo las regiones dinámicas sin redibujar toda la pantalla ---
inline void actualizarPantallaMusica() {
  static unsigned long lastElapsedSec = (unsigned long)(-1);
  static MusicaEstado lastEstado = PAUSE; // para detectar cambio de icono
  static int lastAction = -1;
  static int lastVol = -1;
  static DPadDir lastDir = DPAD_NONE; // para sólo reaccionar a transiciones

  int coverW = 96;
  int ax = 30;
  // aseguro tamaño antes de medir texto
  tft.setTextSize(3);
  int titleX = max((tft.width() - tft.textWidth(cancionActual)) / 2, ax + coverW + 10);

  int pbX = titleX;
  int pbY = 120;
  int pbW = tft.width() - titleX - 20;
  int pbH = 8;

  // calcular elapsed correctamente:
  // CONVENCIÓN: cuando PLAY => musicaStartTime = startTimestamp (ms)
  //             cuando PAUSE => musicaStartTime = storedElapsedMs
  unsigned long elapsedSec;
  if (estadoMusica == PLAY) {
    if (musicaStartTime == 0) elapsedSec = 0;
    else elapsedSec = (millis() - musicaStartTime) / 1000;
  } else {
    // pausa: musicaStartTime guarda elapsed ms
    elapsedSec = musicaStartTime / 1000;
  }

  if (musicaDurationSec == 0) musicaDurationSec = max(1UL, elapsedSec);
  if (elapsedSec > musicaDurationSec) elapsedSec = musicaDurationSec;

  if (elapsedSec != lastElapsedSec || estadoMusica != lastEstado) {
    char leftBuf[16], rightBuf[16];
    snprintf(leftBuf, sizeof(leftBuf), "%02d:%02d", (int)(elapsedSec / 60), (int)(elapsedSec % 60));
    snprintf(rightBuf, sizeof(rightBuf), "%02d:%02d", (int)(musicaDurationSec / 60), (int)(musicaDurationSec % 60));

    int lw = tft.textWidth(leftBuf);
    tft.fillRect(titleX, 96, lw + 2, 16, TFT_BLACK);
    tft.setTextSize(2); tft.setTextColor(TFT_YELLOW);
    tft.setCursor(titleX, 96); tft.print(leftBuf);

    int rightX = tft.width() - tft.textWidth(rightBuf) - 10;
    int rw = tft.textWidth(rightBuf);
    tft.fillRect(rightX, 96, rw + 2, 16, TFT_BLACK);
    tft.setTextSize(2); tft.setTextColor(TFT_CYAN);
    tft.setCursor(rightX, 96); tft.print(rightBuf);

    int fillW = (int)((uint64_t)elapsedSec * pbW / musicaDurationSec);
    // pintar eficientemente
    if (fillW > 0) tft.fillRect(pbX, pbY, fillW, pbH, TFT_ORANGE);
    if (fillW < pbW) tft.fillRect(pbX + fillW, pbY, pbW - fillW, pbH, TFT_DARKGREY);

    lastElapsedSec = elapsedSec;
    // NOTAR: no actualizamos lastEstado aquí porque lo usamos para forzar repintado del icono cuando cambia
  }

  // navegación joystick: solo reaccionar a transiciones L/R (ignorar U/D aquí)
  DPadDir dir = detectarDireccionJoystick();
   if (barraActiva) dir = DPAD_NONE;
  if (dir == DPAD_LEFT && lastDir != DPAD_LEFT) {
    musicaAccionIndex = (musicaAccionIndex - 1 + 5) % 5;
    delay(150);
  } else if (dir == DPAD_RIGHT && lastDir != DPAD_RIGHT) {
    musicaAccionIndex = (musicaAccionIndex + 1) % 5;
    delay(150);
  }

  // actualizar resaltado botones si cambió índice
  if (musicaAccionIndex != lastAction) {
    const char* acciones[] = {"<<", estadoMusica == PLAY ? "||" : ">", ">>", "Vol-", "Vol+"};
    // repintar anterior como normal
    if (lastAction >= 0 && lastAction < 5) {
      int x = 60 + lastAction * 80;
      int y = 220;
      tft.fillRoundRect(x, y, 70, 40, 8, TFT_DARKGREY);
      tft.setTextSize(3); tft.setTextColor(TFT_WHITE);
      int tw = tft.textWidth(acciones[lastAction]);
      tft.setCursor(x + (70 - tw) / 2, y + 6); tft.print(acciones[lastAction]);
    }
    // dibujar nuevo seleccionado
    if (musicaAccionIndex >= 0 && musicaAccionIndex < 5) {
      int x = 60 + musicaAccionIndex * 80;
      int y = 220;
      tft.fillRoundRect(x, y, 70, 40, 8, TFT_ORANGE);
      tft.setTextSize(3); tft.setTextColor(TFT_WHITE);
      const char* accionesCurr[] = {"<<", estadoMusica == PLAY ? "||" : ">", ">>", "Vol-", "Vol+"};
      int tw = tft.textWidth(accionesCurr[musicaAccionIndex]);
      tft.setCursor(x + (70 - tw) / 2, y + 6); tft.print(accionesCurr[musicaAccionIndex]);
    }
    lastAction = musicaAccionIndex;
  }

  // Si cambió el estado PLAY/PAUSE, actualizar el icono central inmediatamente
  if (estadoMusica != lastEstado) {
    const char* centro = estadoMusica == PLAY ? "||" : ">";
    int x = 60 + 1 * 80;
    int y = 220;
    // repintar botón central acorde al estado (respetando si está seleccionado)
    tft.fillRoundRect(x, y, 70, 40, 8, musicaAccionIndex == 1 ? TFT_ORANGE : TFT_DARKGREY);
    tft.setTextSize(3); tft.setTextColor(TFT_WHITE);
    int tw = tft.textWidth(centro);
    tft.setCursor(x + (70 - tw) / 2, y + 6); tft.print(centro);
    lastEstado = estadoMusica;
  }

  // selección con botón
  if (leerBoton(BTN_1)) {
    switch (musicaAccionIndex) {
      case 0: {
        if (estadoMusica == PLAY) {
          unsigned long elapsed = (millis() - musicaStartTime) / 1000;
          elapsed = (elapsed > 10) ? (elapsed - 10) : 0;
          musicaStartTime = millis() - elapsed * 1000;
        } else {
          unsigned long elapsed = musicaStartTime / 1000;
          elapsed = (elapsed > 10) ? (elapsed - 10) : 0;
          musicaStartTime = elapsed * 1000;
        }
        break;
      }
      case 1:
        if (estadoMusica == PLAY) {
          estadoMusica = PAUSE;
          // guardar elapsed (ms)
          musicaStartTime = (millis() - musicaStartTime);
        } else {
          estadoMusica = PLAY;
          // convertir stored elapsed a start timestamp
          musicaStartTime = millis() - musicaStartTime;
        }
        // el cambio de estado será detectado arriba y repintará el icono
        break;
      case 2: {
        if (estadoMusica == PLAY) {
          unsigned long elapsed = (millis() - musicaStartTime) / 1000;
          elapsed = min(elapsed + 10, musicaDurationSec);
          musicaStartTime = millis() - elapsed * 1000;
        } else {
          unsigned long elapsed = musicaStartTime / 1000;
          elapsed = min(elapsed + 10, musicaDurationSec);
          musicaStartTime = elapsed * 1000;
        }
        break;
      }
      case 3: volumen = max(volumen - 1, 0); break;
      case 4: volumen = min(volumen + 1, 10); break;
    }
    delay(200);
  }

  if (volumen != lastVol) {
    tft.fillRect(200, 270, 80, 16, TFT_BLACK);
    tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
    tft.setCursor(200, 270); tft.printf("Vol: %d", volumen);
    lastVol = volumen;
  }

  lastDir = dir;
}
 // ...existing code...