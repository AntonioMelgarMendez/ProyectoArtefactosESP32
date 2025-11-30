#pragma once
#include <TFT_eSPI.h>
#include <vector>
#include <WiFi.h>
#include "utils.h"
#include "joystick.h"

// --- LIBRERÍAS DE AUDIO ---
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include <AudioTools.h>
#include <BluetoothA2DPSink.h>

extern TFT_eSPI tft;
extern bool barraActiva;
extern int navIndex;

// PUNTEROS GLOBALES
extern AudioGeneratorMP3 *mp3;
extern AudioOutputI2S *out;
extern AudioFileSourceSD *file;
extern BluetoothA2DPSink *a2dp_sink;
extern VolumeStream *gain_stream;

extern int modoArranque;

extern std::vector<String> playlist;
extern int songIndex;
extern bool isPaused;
extern float currentVolume;

extern String btTitle;
extern String btArtist;
extern String btAlbum;
extern bool btMetadataChanged;

#define SD_CS_PIN 5
#define PIN_AMP_ENABLE 12
#define I2S_BCK 14
#define I2S_LRC 27
#define I2S_DOUT 26

enum AudioMode { MODE_SD, MODE_BT };
extern AudioMode currentAudioMode;

extern void playCurrentSong(bool autoPlay);

enum MusicView { VIEW_PLAYER, VIEW_LIST };
MusicView currentView = VIEW_PLAYER;

int musicaAccionIndex = 3;

int listCursor = 0;
int listScrollOffset = 0;
const int ITEMS_PER_PAGE = 8;
const int LIST_START_Y = 50;
const int LIST_ITEM_H = 30;

unsigned long lastNavTime = 0;
unsigned long lastBtnTime = 0;

void cambiarModoAudio() {
  digitalWrite(PIN_AMP_ENABLE, LOW);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(100, 220);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  
  if (currentAudioMode == MODE_SD) {
      tft.print("CAMBIANDO A BLUETOOTH...");
      modoArranque = 1;
  } else {
      tft.print("CAMBIANDO A SD...");
      modoArranque = 0;
  }
  
  delay(1000);
  ESP.restart();
}

// -----------------------------------------------------------------------
// FUNCIONES DE DIBUJO (UI)
// -----------------------------------------------------------------------

// --- NUEVA FUNCIÓN AGREGADA ---
// -----------------------------------------------------------------------
// DIBUJO NOTA MUSICAL ESTÉTICA
// -----------------------------------------------------------------------
void dibujarNotaMusical(int x, int y, uint16_t color) {
  // 1. Fondo circular suave
  tft.fillCircle(x + 15, y + 15, 38, TFT_DARKGREY); 
  tft.drawCircle(x + 15, y + 15, 38, TFT_WHITE); // Borde blanco fino

  // 2. Ajuste de coordenadas relativas para centrar
  int nx = x - 5; 
  int ny = y + 5;

  // 3. Cabezas de las notas (Ovaladas para efecto musical)
  tft.fillEllipse(nx, ny + 20, 7, 5, color);       // Nota Izquierda
  tft.fillEllipse(nx + 25, ny + 15, 7, 5, color);  // Nota Derecha (un poco más alta)

  // 4. Plicas (Palos verticales)
  // Palo Izquierdo
  tft.fillRect(nx + 5, ny - 10, 3, 32, color);
  // Palo Derecho
  tft.fillRect(nx + 30, ny - 15, 3, 32, color);

  // 5. Barra de unión (Gruesa y con pendiente)
  // Usamos varios rectángulos o triángulos para simular inclinación
  tft.fillTriangle(nx + 5, ny - 10, nx + 33, ny - 15, nx + 33, ny - 7, color);
  tft.fillTriangle(nx + 5, ny - 10, nx + 5, ny - 2, nx + 33, ny - 7, color);
}

void dibujarBotonIndividual(int index, bool selected) {
  digitalWrite(SD_CS_PIN, HIGH); tft.startWrite();
  uint16_t color = selected ? TFT_ORANGE : TFT_DARKGREY;
  tft.setTextColor(TFT_WHITE);
  if (index == 0) { tft.fillRoundRect(20, 160, 40, 40, 6, color); tft.setTextSize(3); tft.setCursor(28, 168); tft.print("-"); }
  else if (index == 1) { tft.fillRoundRect(420, 160, 40, 40, 6, color); tft.setTextSize(3); tft.setCursor(428, 168); tft.print("+"); }
  else {
    const char* labels[] = {"", "", "<<", isPaused ? ">" : "||", ">>", "LIST", "MOD"};
    if (currentAudioMode == MODE_BT && a2dp_sink != NULL) {
        if(a2dp_sink->get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) labels[3] = "||";
        else labels[3] = ">";
        labels[5] = "---";
        labels[6] = "BT";
    } else {
        labels[6] = "SD";
    }
    int i_row = index - 2; int btnW = 75; int gap = 5; int bx = 20 + i_row * (btnW + gap); int by = 220;
    tft.fillRoundRect(bx, by, btnW, 50, 8, color); tft.setTextSize(2); int tw = tft.textWidth(labels[index]); tft.setCursor(bx + (btnW - tw)/2, by + 18); tft.print(labels[index]);
  }
  tft.endWrite();
}

void dibujarBarraVolumen() {
  digitalWrite(SD_CS_PIN, HIGH); tft.startWrite();
  tft.drawRect(70, 175, 340, 12, TFT_DARKGREY); tft.fillRect(72, 177, 336, 8, TFT_BLACK);
  int volW = (int)(currentVolume * 336.0); if(volW > 336) volW = 336; tft.fillRect(72, 177, volW, 8, TFT_CYAN); tft.endWrite();
}

void actualizarMetadataCancion() {
  digitalWrite(SD_CS_PIN, HIGH); tft.startWrite(); 
  
  // --- CORRECCIÓN AQUÍ: ---
  // Antes empezaba en 0 y borraba el icono. 
  // Ahora empieza en 130 para respetar el área del icono (que termina en 120).
  tft.fillRect(130, 60, 350, 80, TFT_BLACK); 

  tft.setTextSize(2); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  
  if (currentAudioMode == MODE_SD) {
    String nombre = (playlist.size() > 0) ? playlist[songIndex] : "Vacio"; nombre.replace(".mp3", ""); 
    if(nombre.length() > 18) nombre = nombre.substring(0, 18) + "...";
    tft.setCursor(135, 70); tft.print(nombre); 
    tft.setTextColor(TFT_ORANGE, TFT_BLACK); tft.setTextSize(1); tft.setCursor(135, 100);
    if(playlist.size() > 0) tft.printf("Pista %d / %d", songIndex + 1, playlist.size());
  } else {
    String titleDisplay = btTitle; if(titleDisplay.length() > 18) titleDisplay = titleDisplay.substring(0, 18) + "...";
    tft.setCursor(135, 70); tft.print(titleDisplay); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(1); tft.setCursor(135, 100); 
    tft.printf("%s - %s", btArtist.c_str(), btAlbum.c_str());
  }
  tft.endWrite();
}

void drawPlayerView(bool fullRedraw) {
  if (fullRedraw) {
    digitalWrite(SD_CS_PIN, HIGH); tft.startWrite(); tft.fillRect(0, 40, 480, 280, TFT_BLACK);
    
    // --- MODIFICADO: DIBUJAR NOTA MUSICAL EN SD ---
  if (currentAudioMode == MODE_SD) {
        // Fondo del recuadro de carátula
        tft.fillRoundRect(30, 60, 90, 90, 8, TFT_DARKGREY);
        tft.drawRoundRect(30, 60, 90, 90, 8, TFT_WHITE);
        dibujarNotaMusical(60, 90, TFT_YELLOW); 
    } 
    else {
        // Icono BT (sin cambios)
        tft.fillRoundRect(400, 60, 50, 50, 8, TFT_BLUE);
        tft.setTextColor(TFT_WHITE); tft.setTextSize(3);
        tft.setCursor(415, 75); tft.print("B");
    }
    
    tft.endWrite(); dibujarBarraVolumen();
    for (int i = 0; i < 7; i++) dibujarBotonIndividual(i, (i == musicaAccionIndex));
  }
  actualizarMetadataCancion();
}

void drawListRow(int visualIndex, int playlistIndex, bool selected, bool useTransaction) {
    int y = LIST_START_Y + visualIndex * LIST_ITEM_H;
    if (useTransaction) { digitalWrite(SD_CS_PIN, HIGH); tft.startWrite(); }
    uint16_t bg = selected ? TFT_ORANGE : TFT_BLACK; uint16_t fg = selected ? TFT_BLACK : TFT_WHITE; tft.fillRect(0, y, 480, LIST_ITEM_H, bg);
    if (playlistIndex < playlist.size()) { String nombre = playlist[playlistIndex]; nombre.replace(".mp3", ""); if(nombre.length() > 35) nombre = nombre.substring(0, 35) + "..."; tft.setTextColor(fg, bg); tft.setTextSize(2); tft.setCursor(10, y + 6); tft.print(nombre); if (playlistIndex == songIndex) { tft.setCursor(440, y + 6); tft.setTextColor(TFT_GREEN, bg); tft.print("<"); } }
    if (useTransaction) tft.endWrite();
}
void drawListView(bool fullClear) {
    digitalWrite(SD_CS_PIN, HIGH); tft.startWrite();
    if (fullClear) { tft.fillRect(0, 40, 480, 280, TFT_BLACK); tft.fillRect(0, 40, 480, 10, TFT_NAVY); }
    if (playlist.size() == 0) { tft.setCursor(10, 60); tft.setTextColor(TFT_RED); tft.print("Vacia"); tft.endWrite(); return; }
    for (int i = 0; i < ITEMS_PER_PAGE; i++) { int idx = listScrollOffset + i; drawListRow(i, idx, (idx == listCursor), false); }
    tft.endWrite();
}

inline void dibujarPantallaMusica(int nav) { if (currentView == VIEW_PLAYER) drawPlayerView(true); else drawListView(true); }

bool musicaHandleInput(DPadDir dir, bool btn1, bool btn2) {
  unsigned long now = millis();
  static float lastVolDrawn = -1.0; 

  if (currentView == VIEW_PLAYER) {
    if (abs(lastVolDrawn - currentVolume) > 0.01) { dibujarBarraVolumen(); lastVolDrawn = currentVolume; }
    if (now - lastNavTime > 150) {
      int prevIndex = musicaAccionIndex; bool moved = false;
      if (dir == DPAD_RIGHT) { if (musicaAccionIndex == 0) musicaAccionIndex = 1; else if (musicaAccionIndex == 1) musicaAccionIndex = 0; else if (musicaAccionIndex < 6) musicaAccionIndex++; else musicaAccionIndex = 2; moved = true; }
      else if (dir == DPAD_LEFT) { if (musicaAccionIndex == 1) musicaAccionIndex = 0; else if (musicaAccionIndex == 0) musicaAccionIndex = 1; else if (musicaAccionIndex > 2) musicaAccionIndex--; else musicaAccionIndex = 6; moved = true; }
      else if (dir == DPAD_DOWN) { if (musicaAccionIndex <= 1) { musicaAccionIndex = 3; moved = true; } else return true; }
      else if (dir == DPAD_UP) { if (musicaAccionIndex >= 2) { musicaAccionIndex = 0; moved = true; } else return false; }
      if (moved) { dibujarBotonIndividual(prevIndex, false); dibujarBotonIndividual(musicaAccionIndex, true); lastNavTime = now; return true; }
    }

    if (btn1 && (now - lastBtnTime > 300)) {
      lastBtnTime = now;
      switch (musicaAccionIndex) {
        case 0: // Vol-
          currentVolume -= 0.05; if(currentVolume<0) currentVolume=0;
          if(currentAudioMode == MODE_SD && !isPaused && out) {
             out->SetGain(currentVolume);
          }
          else if(currentAudioMode == MODE_BT && a2dp_sink) {
             int vol = (int)(currentVolume * 127);
             a2dp_sink->set_volume(vol);
          }
          break;
        case 1: // Vol+
          currentVolume += 0.05; if(currentVolume>1.0) currentVolume=1.0;
          if(currentAudioMode == MODE_SD && !isPaused && out) {
             out->SetGain(currentVolume);
          }
          else if(currentAudioMode == MODE_BT && a2dp_sink) {
             int vol = (int)(currentVolume * 127);
             a2dp_sink->set_volume(vol);
          }
          break;
        case 2: // Prev
          if(currentAudioMode == MODE_SD) { songIndex--; if(songIndex<0) songIndex=playlist.size()-1; playCurrentSong(true); actualizarMetadataCancion(); }
          else if(a2dp_sink) a2dp_sink->previous();
          break;
        case 3: // Play/Pause
          if(currentAudioMode == MODE_SD) {
             isPaused = !isPaused;
             if(isPaused){ if(out)out->SetGain(0); digitalWrite(PIN_AMP_ENABLE, LOW); }
             else { digitalWrite(PIN_AMP_ENABLE, HIGH); if(out)out->SetGain(currentVolume); }
             dibujarBotonIndividual(3, true); 
          } 
          else if(a2dp_sink) {
             // --- MODIFICADO: PAUSA CON RETARDO ---
             if (a2dp_sink->get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) {
                 a2dp_sink->pause(); 
             } else { 
                 a2dp_sink->play(); 
             }
             // Esperar a que el celular responda antes de actualizar el icono
             delay(250);
             dibujarBotonIndividual(3, true);
          }
          break;
        case 4: // Next
          if(currentAudioMode == MODE_SD) { songIndex++; if(songIndex>=playlist.size()) songIndex=0; playCurrentSong(true); actualizarMetadataCancion(); }
          else if(a2dp_sink) a2dp_sink->next();
          break;
        case 5: // List
          if(currentAudioMode == MODE_SD) {
             currentView = VIEW_LIST; listCursor = songIndex; if(listCursor>=ITEMS_PER_PAGE) listScrollOffset=listCursor; else listScrollOffset=0; drawListView(true);
          }
          break;
        case 6: // MODE
          cambiarModoAudio();
          break;
      }
      return true;
    }
  }
  // Lista (Solo SD)
  else if (currentView == VIEW_LIST) {
    if (now - lastNavTime > 150) { if (dir == DPAD_DOWN) { if (listCursor < playlist.size() - 1) { int old = listCursor; listCursor++; if (listCursor >= listScrollOffset + ITEMS_PER_PAGE) { listScrollOffset++; drawListView(false); } else { drawListRow(old - listScrollOffset, old, false, false); drawListRow(listCursor - listScrollOffset, listCursor, true, false); tft.endWrite(); } lastNavTime = now; } return true; }
    if (dir == DPAD_UP) { if (listCursor > 0) { int old = listCursor; listCursor--; if (listCursor < listScrollOffset) { listScrollOffset--; drawListView(false); } else { drawListRow(old - listScrollOffset, old, false, false); drawListRow(listCursor - listScrollOffset, listCursor, true, false); tft.endWrite(); } lastNavTime = now; return true; } else return false; } }
    if (btn1 && (now - lastBtnTime > 300)) { lastBtnTime = now; songIndex = listCursor; currentView = VIEW_PLAYER; playCurrentSong(true); drawPlayerView(true); return true; }
    if (btn2 && (now - lastBtnTime > 300)) { lastBtnTime = now; currentView = VIEW_PLAYER; drawPlayerView(true); return true; }
    if (dir == DPAD_LEFT || dir == DPAD_RIGHT) return true;
  }
  return false;
}

inline void actualizarPantallaMusica() {
  static int lastSongIdx = -1;
  if (currentAudioMode == MODE_SD && songIndex != lastSongIdx) {
      if (currentView == VIEW_PLAYER) actualizarMetadataCancion(); else drawListView(false);
      lastSongIdx = songIndex;
  }
  if (currentAudioMode == MODE_BT && btMetadataChanged) {
      if (currentView == VIEW_PLAYER) actualizarMetadataCancion();
      btMetadataChanged = false;
  }
}