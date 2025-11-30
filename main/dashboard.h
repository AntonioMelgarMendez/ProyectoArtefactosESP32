#pragma once
#include <TFT_eSPI.h>
#include "utils.h"
#include "sensores.h"

extern TFT_eSPI tft;
extern String fraseDia;
extern float temperatura, humedad;
extern float internetTempC;
extern String internetDesc;
extern bool internetUpdated;
extern float internetPrecipMm;
extern int internetPrecipProb;
extern float internetWind;
extern int internetWeatherCode;

extern String newsHeadline;   
extern bool newsUpdated;      

// Dibujar la base (cajas estáticas)
inline void dibujarBaseDashboard(int navIndex) {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex);

  tft.fillRoundRect(20, 60, 240, 50, 6, TFT_BLACK); tft.drawRoundRect(20, 60, 240, 50, 6, TFT_CYAN);
  tft.fillRoundRect(270, 60, 180, 50, 6, TFT_NAVY); tft.drawRoundRect(270, 60, 180, 50, 6, TFT_CYAN);

  tft.fillRoundRect(20, 120, 200, 100, 6, TFT_DARKGREEN); tft.drawRoundRect(20, 120, 200, 100, 6, TFT_CYAN);

  tft.fillRoundRect(230, 120, 245, 100, 6, TFT_BLACK); tft.drawRoundRect(230, 120, 245, 100, 6, TFT_ORANGE);

  tft.fillRoundRect(20, 250, 440, 40, 6, TFT_NAVY); tft.drawRoundRect(20, 250, 440, 40, 6, TFT_CYAN);
}


// reloj: Se añade parámetro 'forzar'
inline void actualizarRelojFecha(bool forzar = false) {
  struct tm t;
  if(!getLocalTime(&t)) return;

  static int lastSecond = -1;
  // Si NO forzamos Y el segundo es el mismo, salimos
  if (!forzar && t.tm_sec == lastSecond) return;
  lastSecond = t.tm_sec;

  char buf[9]; strftime(buf,sizeof(buf),"%H:%M:%S",&t);
  tft.fillRect(25, 65, 230, 40, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(3); tft.setCursor(30,70); tft.print(buf);

  static int lastDay = -1;
  // Forzar redibujado de fecha tambien si es necesario
  if (forzar || t.tm_mday != lastDay) {
    lastDay = t.tm_mday;
    char buf2[16]; strftime(buf2,sizeof(buf2),"%d/%m/%Y",&t);
    tft.fillRect(275, 65, 170, 40, TFT_NAVY);
    tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(280,75); tft.print(buf2);
  }
}

// calendario: Se añade parámetro 'forzar'
inline void actualizarCalendario(bool forzar = false) {
  struct tm timeinfo; if(!getLocalTime(&timeinfo)) return;
  static int lastMonth = -1, lastYear = -1, lastToday = -1;
  int year=timeinfo.tm_year+1900, month=timeinfo.tm_mon+1, today=timeinfo.tm_mday;
  
  // Si NO forzamos Y la fecha es identica, no hacemos nada
  if (!forzar && year==lastYear && month==lastMonth && today==lastToday) return;
  
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

// frase del día: Se añade parámetro 'forzar'
inline void actualizarFrase(bool forzar = false) {
  static String lastFrase = "";
  if (!forzar && fraseDia == lastFrase) return;
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

// datos sensor: Se añade parámetro 'forzar'
inline void actualizarDatosSensor(bool forzar = false) {
  const int P_X = 230;
  const int P_Y = 120;
  const int P_W = 245;
  const int P_H = 100;

  if (isnan(temperatura) || isnan(humedad)) return;
  if (isnan(internetTempC) || isnan(internetPrecipMm) || isnan(internetWind)) return;

  static float lastTemp = NAN, lastHum = NAN;
  static float lastInternetTemp = NAN;
  static float lastPrecip = NAN;
  static float lastWind = NAN;
  static int lastWeatherCode = -9999;

  bool needSensor = false;
  if (forzar || isnan(lastTemp) || fabs(temperatura - lastTemp) >= 0.2) { lastTemp = temperatura; needSensor = true; }
  if (forzar || isnan(lastHum)  || fabs(humedad - lastHum) >= 1.0) { lastHum = humedad; needSensor = true; }

  bool needInternet = internetUpdated ||
    forzar ||
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

  // Si necesitamos redibujar (por cambio o por forzado)
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

  // Noticia: Se añade parametro forzar
  static String lastHeadline = "";
  if (forzar || newsHeadline != lastHeadline) {
    lastHeadline = newsHeadline;
    tft.startWrite();
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

    tft.fillRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, BG_COLOR);
    tft.drawRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, TFT_CYAN);

    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(CLIP_X, TICKER_BASELINE);
    String toShow = newsHeadline.length() > 0 ? newsHeadline : "Sin titulares";
    if (tft.textWidth(toShow) > CLIP_W) {
      int maxLen = toShow.length();
      while (maxLen > 0 && tft.textWidth(toShow.substring(0, maxLen) + "...") > CLIP_W) maxLen--;
      toShow = toShow.substring(0, maxLen) + "...";
    }
    tft.print(toShow);
    tft.endWrite();
  }
}