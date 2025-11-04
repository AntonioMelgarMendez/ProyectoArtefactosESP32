#pragma once
#include <TFT_eSPI.h>
#include "utils.h"
#include "sensores.h"

extern TFT_eSPI tft;
extern String fraseDia;
extern float temperatura, humedad;

// internet / news externs (definidos en main.ino)
extern float internetTempC;
extern String internetDesc;
extern bool internetUpdated;
extern float internetPrecipMm;
extern int internetPrecipProb;
extern float internetWind;
extern int internetWeatherCode;

extern String newsHeadline;   // último titular recibido por RSS
extern bool newsUpdated;      // flag que indica llegada de un nuevo titular

// Dibujar la base sólo cuando se entra o cambia navIndex
inline void dibujarBaseDashboard(int navIndex) {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex);

  tft.fillRoundRect(20, 60, 240, 50, 6, TFT_BLACK); tft.drawRoundRect(20, 60, 240, 50, 6, TFT_CYAN);
  tft.fillRoundRect(270, 60, 180, 50, 6, TFT_NAVY); tft.drawRoundRect(270, 60, 180, 50, 6, TFT_CYAN);

  tft.fillRoundRect(20, 120, 200, 100, 6, TFT_DARKGREEN); tft.drawRoundRect(20, 120, 200, 100, 6, TFT_CYAN);

  tft.fillRoundRect(230, 120, 245, 100, 6, TFT_BLACK); tft.drawRoundRect(230, 120, 245, 100, 6, TFT_ORANGE);

  tft.fillRoundRect(20, 250, 440, 40, 6, TFT_NAVY); tft.drawRoundRect(20, 250, 440, 40, 6, TFT_CYAN);
}

// reloj: redibujar sólo cuando cambie el segundo
inline void actualizarRelojFecha() {
  struct tm t;
  if(!getLocalTime(&t)) return;

  static int lastSecond = -1;
  if (t.tm_sec == lastSecond) return;
  lastSecond = t.tm_sec;

  char buf[9]; strftime(buf,sizeof(buf),"%H:%M:%S",&t);
  tft.fillRect(25, 65, 230, 40, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(3); tft.setCursor(30,70); tft.print(buf);

  static int lastDay = -1;
  if (t.tm_mday != lastDay) {
    lastDay = t.tm_mday;
    char buf2[16]; strftime(buf2,sizeof(buf2),"%d/%m/%Y",&t);
    tft.fillRect(275, 65, 170, 40, TFT_NAVY);
    tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(280,75); tft.print(buf2);
  }
}

// calendario: redibujar sólo cuando cambie día/mes/año o si se fuerza
inline void actualizarCalendario() {
  struct tm timeinfo; if(!getLocalTime(&timeinfo)) return;
  static int lastMonth = -1, lastYear = -1, lastToday = -1;
  int year=timeinfo.tm_year+1900, month=timeinfo.tm_mon+1, today=timeinfo.tm_mday;
  if (year==lastYear && month==lastMonth && today==lastToday) return;
  lastYear = year; lastMonth = month; lastToday = today;

  tm firstDay=timeinfo; firstDay.tm_mday=1; mktime(&firstDay);
  int startWeekday=firstDay.tm_wday;
  int diasMes[]={31,28,31,30,31,30,31,31,30,31,30,31};
  if(month==2 && ((year%4==0 && year%100!=0) || year%400==0)) diasMes[1]=29;
  int numDias=diasMes[month-1];

  int contX = 20, contY = 120, contW = 200, contH = 100;
  int pad = 6;
  int availW = contW - pad*2;
  int availH = contH - pad*2;

  int cellSize = min(availW / 7, availH / 6);
  if (cellSize < 8) cellSize = 8;

  int calW = cellSize * 7;
  int calH = cellSize * 6;
  int drawX = contX + (contW - calW) / 2;
  int drawY = contY + (contH - calH) / 2;

  tft.fillRect(drawX, drawY, calW, calH, TFT_DARKGREEN);

  int day=1;
  for(int row=0; row<6; row++){
    for(int col=0; col<7; col++){
      int pos=row*7+col;
      int x=drawX+col*cellSize;
      int y=drawY+row*cellSize;
      if(pos>=startWeekday && day<=numDias){
        if(day==today){
          tft.fillRect(x+1,y+1,cellSize-2,cellSize-2,TFT_YELLOW);
          tft.setTextColor(TFT_BLACK);
        } else {
          tft.fillRect(x+1,y+1,cellSize-2,cellSize-2,TFT_DARKGREEN);
          tft.setTextColor(TFT_WHITE);
        }
        tft.setTextSize(1);
        tft.setCursor(x + 3, y + 3);
        tft.print(day);
        day++;
      } else {
        tft.fillRect(x+1,y+1,cellSize-2,cellSize-2,TFT_DARKGREEN);
      }
    }
  }
  for(int i=0;i<=7;i++) tft.drawLine(drawX+i*cellSize, drawY, drawX+i*cellSize, drawY+calH, TFT_DARKGREY);
  for(int i=0;i<=6;i++) tft.drawLine(drawX, drawY+i*cellSize, drawX+calW, drawY+i*cellSize, TFT_DARKGREY);
}

// frase del día: sólo redibujar cuando cambie
inline void actualizarFrase() {
  static String lastFrase = "";
  if (fraseDia == lastFrase) return;
  lastFrase = fraseDia;

  tft.fillRect(351,121,108,118,TFT_BLACK);
  tft.setTextColor(TFT_ORANGE); tft.setTextSize(1); tft.setCursor(355,125); tft.print("Frase:");
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  int maxChars=14, lineH=12;
  for(int i=0;i<fraseDia.length();i+=maxChars){
    tft.setCursor(355,140+(i/maxChars)*lineH);
    tft.print(fraseDia.substring(i,i+maxChars));
  }
}

// datos sensor + internet + ticker optimizado (actualiza sólo regiones necesarias)
inline void actualizarDatosSensor() {
  const int P_X = 230;
  const int P_Y = 120;
  const int P_W = 245;
  const int P_H = 100;

  // --- Validación de datos antes de dibujar ---
  if (isnan(temperatura) || isnan(humedad)) return;
  if (isnan(internetTempC) || isnan(internetPrecipMm) || isnan(internetWind)) return;

  static float lastTemp = NAN, lastHum = NAN;
  static float lastInternetTemp = NAN;
  static float lastPrecip = NAN;
  static float lastWind = NAN;
  static int lastWeatherCode = -9999;

  bool needSensor = false;
  if (isnan(lastTemp) || fabs(temperatura - lastTemp) >= 0.2) { lastTemp = temperatura; needSensor = true; }
  if (isnan(lastHum)  || fabs(humedad - lastHum) >= 1.0) { lastHum = humedad; needSensor = true; }

  bool needInternet = internetUpdated ||
    isnan(lastInternetTemp) || fabs(internetTempC - lastInternetTemp) >= 0.2 ||
    isnan(lastPrecip) || fabs(internetPrecipMm - lastPrecip) >= 0.1 ||
    isnan(lastWind) || fabs(internetWind - lastWind) >= 0.2 ||
    internetWeatherCode != lastWeatherCode;

  if (internetUpdated) internetUpdated = false;
  if (needInternet) {
    lastInternetTemp = internetTempC;
    lastPrecip = internetPrecipMm;
    lastWind = internetWind;
    lastWeatherCode = internetWeatherCode;
  }

  if (needSensor || needInternet) {
    tft.startWrite();
    tft.fillRect(P_X+5, P_Y+5, P_W-10, P_H-10, TFT_BLACK);

    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(P_X + 8, P_Y + 8);
    tft.print("Sensor local:");

    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(P_X + 8, P_Y + 26);
    tft.print("T:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(P_X + 36, P_Y + 26);
    tft.printf("%4.1fC", temperatura);

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(P_X + 8, P_Y + 50);
    tft.print("H:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(P_X + 36, P_Y + 50);
    tft.printf("%3.0f%%", humedad);

    int insetX = P_X + 120;
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(insetX, P_Y + 8);
    tft.print("Clima (Open‑Meteo):");

    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(insetX, P_Y + 26);
    tft.print("Temp:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(insetX + 48, P_Y + 26);
    tft.printf("%4.1fC", internetTempC);

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(insetX, P_Y + 44);
    tft.print("P:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(insetX + 18, P_Y + 44);
    tft.printf("%4.1fmm", internetPrecipMm);

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(insetX, P_Y + 62);
    tft.print("V:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(insetX + 18, P_Y + 62);
    tft.printf("%4.1fm/s", internetWind);

    tft.setTextSize(1);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(insetX, P_Y + 82);
    tft.print("WC:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(insetX + 26, P_Y + 82);
    tft.print(String(internetWeatherCode));
    tft.endWrite();
  }

  // --- Ticker de noticias con control robusto de duplicados ---
  const int NEWS_MAX = 8;
  const int BAR_X = 20;
  const int BAR_Y = 250;
  const int BAR_W = 440;
  const int BAR_H = 40;
  const int CLIP_X = BAR_X + 6;
  const int CLIP_Y = BAR_Y + 6;
  const int CLIP_W = BAR_W - 24;
  const int TICKER_BASELINE = CLIP_Y + 18;
  const uint16_t BG_COLOR = TFT_NAVY;
  const uint16_t TEXT_COLOR = TFT_WHITE;
  const unsigned long tickerSpeedMs = 30;
  const int stepPx = 1;
  const unsigned long pauseMs = 1200;

  static String newsList[NEWS_MAX];
  static int newsCount = 0;
  static int newsIdx = 0;
  static int xPos = 0;
  static unsigned long lastMove = 0;
  static bool paused = false;
  static unsigned long pauseStart = 0;
  static bool initialized = false;

  static bool bgNeedsRedraw = true;
  static bool lastEmptyPrinted = false;

  static bool slideMode = false;
  static int slideOffset = CLIP_Y + BAR_H - CLIP_Y;
  static unsigned long slideLast = 0;
  static bool slidePaused = false;
  static unsigned long slidePauseStart = 0;

  static int prevXPos = INT32_MIN;
  static int prevNewsIdx = -1;
  static int prevSlideOffset = INT32_MIN;
  static bool prevSlideMode = false;
  static String prevVis = "";

  static unsigned long lastNewsAddMs = 0;
  const unsigned long NEWS_ADD_DEBOUNCE_MS = 300;

  // --- Agregar noticia solo si no existe ---
  if (newsUpdated) {
    newsUpdated = false;
    String candidate = newsHeadline;
    candidate.trim();
    if (candidate.length() > 0) {
      unsigned long nowMs = millis();
      if (nowMs - lastNewsAddMs >= NEWS_ADD_DEBOUNCE_MS) {
        bool duplicate = false;
        for (int i = 0; i < newsCount; ++i) {
          if (newsList[i] == candidate) { duplicate = true; break; }
        }
        if (!duplicate) {
          if (newsCount < NEWS_MAX) newsList[newsCount++] = candidate;
          else {
            for (int i = 0; i < NEWS_MAX-1; ++i) newsList[i] = newsList[i+1];
            newsList[NEWS_MAX-1] = candidate;
          }
          lastNewsAddMs = nowMs;
          slideMode = true;
          slideOffset = BAR_H + 8;
          slideLast = nowMs;
          slidePaused = false;
          bgNeedsRedraw = true;
          if (!initialized) {
            initialized = true;
            newsIdx = 0;
            xPos = tft.width();
            paused = false;
            lastMove = nowMs;
          }
        }
      }
    }
  }

  if (bgNeedsRedraw) {
    tft.startWrite();
    tft.fillRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, BG_COLOR);
    tft.drawRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, TFT_CYAN);
    tft.endWrite();
    bgNeedsRedraw = false;
    lastEmptyPrinted = false;
    prevXPos = INT32_MIN;
    prevVis = "";
  }

  if (newsCount == 0) {
    if (!lastEmptyPrinted) {
      tft.startWrite();
      tft.setTextSize(2);
      tft.setTextColor(TEXT_COLOR);
      tft.fillRect(CLIP_X, CLIP_Y, CLIP_W, BAR_H - 8, BG_COLOR);
      tft.setCursor(CLIP_X, TICKER_BASELINE);
      tft.print("Sin titulares");
      tft.endWrite();
      lastEmptyPrinted = true;
    }
    return;
  }

  if (newsIdx >= newsCount) newsIdx = 0;
  if (newsIdx < 0) newsIdx = 0;

  String cur = newsList[newsIdx];
  String sep = "   .   ";
  String displayText = cur + sep;

  tft.setTextSize(2);
  int fullW = tft.textWidth(displayText);

  if (!initialized) {
    initialized = true;
    xPos = tft.width();
    lastMove = millis();
    paused = false;
    bgNeedsRedraw = true;
  }

  unsigned long now = millis();

  if (slideMode) {
    bool changed = false;
    if (!slidePaused) {
      if (now - slideLast >= 25) {
        slideLast = now;
        int oldOffset = slideOffset;
        if (slideOffset > 0) slideOffset -= 4;
        if (slideOffset <= 0) {
          slideOffset = 0;
          slidePaused = true;
          slidePauseStart = now;
        }
        changed = (slideOffset != oldOffset);
      }
    } else {
      if (now - slidePauseStart >= pauseMs) {
        slideMode = false;
        xPos = tft.width();
        lastMove = now;
        paused = true;
        pauseStart = now;
        changed = true;
      }
    }

    if (prevSlideMode != slideMode || slideOffset != prevSlideOffset || changed) {
      tft.startWrite();
      tft.fillRect(CLIP_X, CLIP_Y, CLIP_W, BAR_H - 8, BG_COLOR);
      int textW = tft.textWidth(cur);
      int drawX = CLIP_X + max(0, (CLIP_W - textW) / 2);
      int drawY = CLIP_Y + slideOffset;
      if (drawY < CLIP_Y) drawY = CLIP_Y;
      if (drawY > CLIP_Y + BAR_H - 8) drawY = CLIP_Y + BAR_H - 8;
      tft.setTextColor(TEXT_COLOR);
      tft.setCursor(drawX, drawY);
      tft.print(cur);
      tft.endWrite();
      prevSlideOffset = slideOffset;
      prevSlideMode = slideMode;
      prevVis = "";
      prevXPos = INT32_MIN;
    }
    return;
  }

  if (paused) {
    if (now - pauseStart >= pauseMs) paused = false;
  } else {
    if (now - lastMove >= tickerSpeedMs) {
      lastMove = now;
      xPos -= stepPx;
    }
  }

  int pxStart = CLIP_X - xPos;
  if (pxStart < 0) pxStart = 0;
  int charStart = 0;
  int acc = 0;
  int len = displayText.length();
  while (charStart < len) {
    int w = tft.textWidth(displayText.substring(charStart, charStart+1));
    if (acc + w > pxStart) break;
    acc += w;
    ++charStart;
  }
  int pxOffsetInChar = pxStart - acc;
  int acc2 = 0;
  int charEnd = charStart;
  while (charEnd < len) {
    int w = tft.textWidth(displayText.substring(charEnd, charEnd+1));
    if (acc2 + w > CLIP_W + pxOffsetInChar) break;
    acc2 += w;
    ++charEnd;
  }
  if (charEnd <= charStart) charEnd = min(len, charStart + 1);
  String vis = displayText.substring(charStart, charEnd);
  int drawX = CLIP_X - pxOffsetInChar;

  if (xPos != prevXPos || vis != prevVis || newsIdx != prevNewsIdx) {
    tft.startWrite();
    tft.fillRect(CLIP_X, CLIP_Y, CLIP_W, BAR_H - 8, BG_COLOR);
    tft.setTextColor(TEXT_COLOR);
    tft.setTextSize(2);
    tft.setCursor(drawX, TICKER_BASELINE);
    tft.print(vis);
    tft.endWrite();

    prevXPos = xPos;
    prevVis = vis;
    prevNewsIdx = newsIdx;
  }

  if (xPos + fullW < CLIP_X) {
    newsIdx = (newsIdx + 1) % newsCount;
    xPos = tft.width();
    paused = true;
    pauseStart = now;
    prevXPos = INT32_MIN;
    prevVis = "";
  }
}