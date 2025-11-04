#include <TFT_eSPI.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// === CONFIGURACIÓN DE PINES ===
#define JOY_X 34
#define JOY_Y 32
#define JOY_SW 25 
#define BTN_1  17
#define BTN_2  16
#define DHT_PIN 33
#define DHT_TYPE DHT11

// === CREDENCIALES WIFI ===
const char* ssid = "TIGO-4B5C";
const char* password = "4D99ED801413";

// === NTP ===
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -6 * 3600;
const int daylightOffset_sec = 0;

// === INICIALIZACIÓN ===
TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);

// === ESTRUCTURA DE CALIBRACIÓN ===
struct JoystickCal {
  int x_min, x_max, x_center;
  int y_min, y_max, y_center;
  int deadzone;
  bool x_invertido = false;
  bool y_invertido = false;
  bool done = false;
} cal;

// === ESTADOS ===
enum CalState { INIT, CENTER, LEFT, RIGHT, UP, DOWN, COMPLETE };
CalState cal_state = INIT;

// === VARIABLES ===
float temperatura = 0, humedad = 0;
unsigned long lastDHTRead = 0;
const unsigned long DHT_READ_INTERVAL = 2000;

// Frase del día
String fraseDia = "";
unsigned long lastFraseUpdate = 0;
const unsigned long FRASE_UPDATE_INTERVAL = 3600000; // 1 hora

enum DPadDir { DPAD_NONE, DPAD_LEFT, DPAD_RIGHT, DPAD_UP, DPAD_DOWN };

// === MENÚ PRINCIPAL ===
enum Pantalla { DASHBOARD, ALARMAS, MUSICA };
Pantalla pantallaActual = DASHBOARD;
int navIndex = 0; // Para barra de navegación

// === ALARMAS ===
struct Alarma {
  int hora;
  int minuto;
  bool activa;
};
Alarma alarmas[5] = {{7, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}};
int numAlarmas = 1;
int alarmaSeleccionada = 0;
bool mostrarModalAlarma = false;
bool modoAgregarAlarma = false;
int nuevaHora = 7, nuevoMinuto = 0;
int menuAlarmaIndex = 0;

// === MÚSICA ===
enum MusicaEstado { PLAY, PAUSE };
MusicaEstado estadoMusica = PAUSE;
int volumen = 5; // 0-10
int musicaAccionIndex = 0; // Navegación horizontal de acciones
unsigned long musicaStartTime = 0;
String cancionActual = "Cancion Ejemplo";

// =========================================================
//                  FUNCIONES AUXILIARES
// =========================================================
bool leerBoton(int pin) {
  static unsigned long lastPress = 0;
  bool currentState = digitalRead(pin) == LOW;
  if (currentState && millis() - lastPress > 500) {
    lastPress = millis();
    return true;
  }
  return false;
}

int leerPromedio(int pin, int muestras = 20) {
  long suma = 0;
  for (int i = 0; i < muestras; i++) {
    suma += analogRead(pin);
    delay(2);
  }
  return suma / muestras;
}

// =========================================================
//                  CALIBRACIÓN
// =========================================================
void calibrarCentro() { cal.x_center = leerPromedio(JOY_X, 50); cal.y_center = leerPromedio(JOY_Y, 50); }
void calibrarIzquierda() { cal.x_min = leerPromedio(JOY_X); }
void calibrarDerecha() { cal.x_max = leerPromedio(JOY_X); }
void calibrarArriba() { cal.y_min = leerPromedio(JOY_Y); }
void calibrarAbajo() { cal.y_max = leerPromedio(JOY_Y); }

// =========================================================
//                CALIBRACIÓN INTERACTIVA
// =========================================================
void procesoCalibracion() {
  int x_val=leerPromedio(JOY_X,15);
  int y_val=leerPromedio(JOY_Y,15);
  tft.fillRect(30,180,420,30,TFT_BLACK);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(40,185);
  tft.printf("X:%d Y:%d",x_val,y_val);

  if(leerBoton(BTN_1)){
    switch(cal_state){
      case INIT: cal_state=CENTER; tft.fillScreen(TFT_NAVY); tft.setTextColor(TFT_CYAN);
                 tft.setTextSize(3); tft.setCursor(30,140); tft.print("Deja el joystick CENTRADO"); delay(1000); break;
      case CENTER: calibrarCentro(); cal_state=LEFT; tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve IZQUIERDA"); delay(1000); break;
      case LEFT: calibrarIzquierda(); cal_state=RIGHT; tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve DERECHA"); delay(1000); break;
      case RIGHT: calibrarDerecha(); cal_state=UP; tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve ARRIBA"); delay(1000); break;
      case UP: calibrarArriba(); cal_state=DOWN; tft.fillScreen(TFT_NAVY); tft.setCursor(30,140); tft.print("Mueve ABAJO"); delay(1000); break;
      case DOWN: calibrarAbajo(); cal_state=COMPLETE; cal.x_invertido=cal.x_min>cal.x_max; cal.y_invertido=cal.y_min>cal.y_max; cal.done=true; break;
      default: break;
    }
  }
}

// =========================================================
//                 LECTURA DHT11
// =========================================================
void leerDHT11() {
  if(millis()-lastDHTRead>=DHT_READ_INTERVAL){
    float t=dht.readTemperature();
    float h=dht.readHumidity();
    if(!isnan(t)) temperatura=t;
    if(!isnan(h)) humedad=h;
    lastDHTRead=millis();
  }
}

// =========================================================
//             OBTENER FRASE DEL DÍA DE INTERNET
// =========================================================
void obtenerFraseDia() {
  if(millis()-lastFraseUpdate<FRASE_UPDATE_INTERVAL && fraseDia.length()>0) return;
  lastFraseUpdate=millis();
  HTTPClient http;
  http.begin("https://api.quotable.io/random?maxLength=60");
  int code=http.GET();
  if(code==200){
    String payload=http.getString();
    DynamicJsonDocument doc(256);
    deserializeJson(doc,payload);
    fraseDia=doc["content"].as<String>();
  }else fraseDia="Sin conexión";
  http.end();
}

// =========================================================
//         DETECCIÓN DE DIRECCIÓN DEL JOYSTICK
// =========================================================
DPadDir detectarDireccionJoystick() {
  int x_val = leerPromedio(JOY_X, 10);
  int y_val = leerPromedio(JOY_Y, 10);
  if (x_val < cal.x_center - 150) return DPAD_LEFT;
  if (x_val > cal.x_center + 150) return DPAD_RIGHT;
  if (y_val < cal.y_center - 150) return DPAD_UP;
  if (y_val > cal.y_center + 150) return DPAD_DOWN;
  return DPAD_NONE;
}

// =========================================================
//                 BARRA DE NAVEGACIÓN
// =========================================================
void dibujarBarraNavegacion(int seleccionado) {
  tft.fillRect(0, 0, 480, 40, TFT_DARKGREY);
  const char* titulos[] = {"Dashboard", "Alarmas", "Musica"};
  for (int i = 0; i < 3; i++) {
    tft.setTextColor(i == seleccionado ? TFT_ORANGE : TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(30 + i * 150, 10);
    tft.print(titulos[i]);
  }
}

// =========================================================
//                     DASHBOARD
// =========================================================
void dibujarBaseDashboard() {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex);

  // Reloj y Fecha
  tft.fillRoundRect(20, 60, 240, 50, 6, TFT_BLACK); tft.drawRoundRect(20, 60, 240, 50, 6, TFT_CYAN);
  tft.fillRoundRect(270, 60, 180, 50, 6, TFT_NAVY); tft.drawRoundRect(270, 60, 180, 50, 6, TFT_CYAN);

  // Calendario
  tft.fillRoundRect(20, 120, 320, 120, 6, TFT_DARKGREEN); tft.drawRoundRect(20, 120, 320, 120, 6, TFT_CYAN);

  // Frase
  tft.fillRoundRect(350, 120, 110, 120, 6, TFT_BLACK); tft.drawRoundRect(350, 120, 110, 120, 6, TFT_ORANGE);

  // Sensor
  tft.fillRoundRect(20, 250, 440, 40, 6, TFT_NAVY); tft.drawRoundRect(20, 250, 440, 40, 6, TFT_CYAN);
}

void actualizarRelojFecha() {
  struct tm t; if(!getLocalTime(&t)) return;
  char buf[9]; strftime(buf,sizeof(buf),"%H:%M:%S",&t);
  tft.fillRect(25, 65, 230, 40, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(3); tft.setCursor(30,70); tft.print(buf);
  
  char buf2[16]; strftime(buf2,sizeof(buf2),"%d/%m/%Y",&t);
  tft.fillRect(275, 65, 170, 40, TFT_NAVY);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(280,75); tft.print(buf2);
}

void actualizarCalendario() {
  struct tm timeinfo; if(!getLocalTime(&timeinfo)) return;
  int year=timeinfo.tm_year+1900, month=timeinfo.tm_mon+1, today=timeinfo.tm_mday;

  tm firstDay=timeinfo; firstDay.tm_mday=1; mktime(&firstDay);
  int startWeekday=firstDay.tm_wday;
  int diasMes[]={31,28,31,30,31,30,31,31,30,31,30,31};
  if(month==2 && ((year%4==0 && year%100!=0) || year%400==0)) diasMes[1]=29;
  int numDias=diasMes[month-1];

  int cellSize=22; int x0=25, y0=125;

  tft.fillRect(x0, y0, cellSize*7, cellSize*6, TFT_DARKGREEN);

  int day=1;
  for(int row=0; row<6; row++){
    for(int col=0; col<7; col++){
      int pos=row*7+col;
      if(pos>=startWeekday && day<=numDias){
        int x=x0+col*cellSize;
        int y=y0+row*cellSize;
        if(day==today){ tft.fillRect(x+1,y+1,cellSize-2,cellSize-2,TFT_YELLOW); tft.setTextColor(TFT_BLACK);}
        else tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setCursor(x+5,y+5);
        tft.print(day);
        day++;
      }
    }
  }

  for(int i=0;i<=7;i++) tft.drawLine(x0+i*cellSize,y0,x0+i*cellSize,y0+cellSize*6,TFT_DARKGREY);
  for(int i=0;i<=6;i++) tft.drawLine(x0,y0+i*cellSize,x0+cellSize*7,y0+i*cellSize,TFT_DARKGREY);
}

void actualizarFrase() {
  tft.fillRect(351,121,108,118,TFT_BLACK);
  tft.setTextColor(TFT_ORANGE); tft.setTextSize(1); tft.setCursor(355,125); tft.print("Frase:");
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  int maxChars=14, lineH=12;
  for(int i=0;i<fraseDia.length();i+=maxChars){
    tft.setCursor(355,140+(i/maxChars)*lineH);
    tft.print(fraseDia.substring(i,i+maxChars));
  }
}

void actualizarDatosSensor() {
  tft.fillRect(21,251,438,38,TFT_NAVY);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(25,260);
  tft.printf("Temp: %.1f °C",temperatura);
  tft.setTextColor(TFT_CYAN); tft.setCursor(250,260); tft.printf("Hum: %.1f %%",humedad);
}

// =========================================================
//                     ALARMAS
// =========================================================
void dibujarPantallaAlarmas() {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex);

  int menuItems = numAlarmas + 1;
  for (int i = 0; i < numAlarmas; i++) {
    tft.setTextColor(menuAlarmaIndex == i ? TFT_CYAN : TFT_WHITE);
    tft.setCursor(60, 60 + i * 30);
    tft.printf("%d) %02d:%02d %s", i+1, alarmas[i].hora, alarmas[i].minuto, alarmas[i].activa ? "ON" : "OFF");
  }
  tft.setTextColor(menuAlarmaIndex == numAlarmas ? TFT_ORANGE : TFT_WHITE);
  tft.setCursor(60, 60 + numAlarmas * 30);
  tft.print("+ Agregar nueva alarma");

  tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
  tft.setCursor(30, 250); tft.print("Arriba/Abajo: Navegar  BTN: Seleccionar");
}

void dibujarModalAgregarAlarma() {
  tft.fillRoundRect(60, 80, 360, 160, 12, TFT_YELLOW);
  tft.setTextColor(TFT_RED); tft.setTextSize(2);
  tft.setCursor(120, 100); tft.print("Nueva alarma");
  tft.setTextColor(TFT_BLACK); tft.setTextSize(3);
  tft.setCursor(120, 140); tft.printf("%02d:%02d", nuevaHora, nuevoMinuto);
  tft.setTextColor(TFT_BLACK); tft.setTextSize(2);
  tft.setCursor(120, 180); tft.print("Izq/Der: Hora/Min");
  tft.setCursor(120, 210); tft.print("Arriba/Abajo: + / -");
  tft.setCursor(120, 230); tft.print("BTN: Guardar  BTN2: Cancelar");
}

void actualizarPantallaAlarmas() {
  static bool modalDrawn = false;
  if (!modoAgregarAlarma) {
    dibujarPantallaAlarmas();
    modalDrawn = false;
    DPadDir dir = detectarDireccionJoystick();
    if (dir == DPAD_UP) {
      menuAlarmaIndex = (menuAlarmaIndex - 1 + numAlarmas + 1) % (numAlarmas + 1);
      delay(200);
    }
    if (dir == DPAD_DOWN) {
      menuAlarmaIndex = (menuAlarmaIndex + 1) % (numAlarmas + 1);
      delay(200);
    }
    if (leerBoton(BTN_1)) {
      if (menuAlarmaIndex < numAlarmas) {
        alarmas[menuAlarmaIndex].activa = !alarmas[menuAlarmaIndex].activa;
      } else {
        modoAgregarAlarma = true;
        nuevaHora = 7;
        nuevoMinuto = 0;
        modalDrawn = false;
      }
      delay(200);
    }
  } else {
    if (!modalDrawn) { dibujarModalAgregarAlarma(); modalDrawn = true; }
    static bool editHora = true;
    DPadDir dir = detectarDireccionJoystick();
    if (dir == DPAD_LEFT) { editHora = true; delay(200);}
    if (dir == DPAD_RIGHT) { editHora = false; delay(200);}
    if (dir == DPAD_UP) {
      if (editHora) nuevaHora = (nuevaHora + 1) % 24;
      else nuevoMinuto = (nuevoMinuto + 1) % 60;
      dibujarModalAgregarAlarma();
      delay(200);
    }
    if (dir == DPAD_DOWN) {
      if (editHora) nuevaHora = (nuevaHora - 1 + 24) % 24;
      else nuevoMinuto = (nuevoMinuto - 1 + 60) % 60;
      dibujarModalAgregarAlarma();
      delay(200);
    }
    if (leerBoton(BTN_1)) {
      if (numAlarmas < 5) {
        alarmas[numAlarmas].hora = nuevaHora;
        alarmas[numAlarmas].minuto = nuevoMinuto;
        alarmas[numAlarmas].activa = true;
        numAlarmas++;
      }
      modoAgregarAlarma = false;
      menuAlarmaIndex = 0;
      delay(300);
    }
    if (leerBoton(BTN_2)) {
      modoAgregarAlarma = false;
      menuAlarmaIndex = 0;
      delay(300);
    }
  }

  struct tm t; getLocalTime(&t);
  for (int i = 0; i < numAlarmas; i++) {
    if(alarmas[i].activa && t.tm_hour == alarmas[i].hora && t.tm_min == alarmas[i].minuto && !mostrarModalAlarma) {
      mostrarModalAlarma = true;
      alarmaSeleccionada = i;
    }
  }
  if(mostrarModalAlarma) {
    tft.fillRoundRect(80, 100, 320, 100, 12, TFT_YELLOW);
    tft.setTextColor(TFT_RED); tft.setTextSize(3);
    tft.setCursor(120, 130); tft.print("¡ALARMA!");
    tft.setTextColor(TFT_BLACK); tft.setTextSize(2);
    tft.setCursor(120, 170); tft.print("Presiona cualquier boton");
    if(leerBoton(BTN_1) || leerBoton(BTN_2)) {
      mostrarModalAlarma = false;
      alarmas[alarmaSeleccionada].activa = false;
      delay(500);
    }
  }
}

// =========================================================
//                     MÚSICA
// =========================================================
void dibujarPantallaMusica() {
  tft.fillScreen(TFT_BLACK);
  dibujarBarraNavegacion(navIndex);

  // Título y tiempo arriba
  tft.setTextColor(TFT_WHITE); tft.setTextSize(3);
  tft.setCursor(80, 60); tft.print(cancionActual);

  unsigned long elapsed = (estadoMusica == PLAY) ? (millis() - musicaStartTime) / 1000 : musicaStartTime / 1000;
  int min = elapsed / 60;
  int sec = elapsed % 60;
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2);
  tft.setCursor(350, 60); tft.printf("%02d:%02d", min, sec);

  // Acciones en fila horizontal abajo
  const char* acciones[] = {"<<", estadoMusica == PLAY ? "||" : ">", ">>", "Vol-", "Vol+"};
  for (int i = 0; i < 5; i++) {
    int x = 60 + i * 80;
    int y = 220;
    tft.fillRoundRect(x, y, 70, 40, 8, musicaAccionIndex == i ? TFT_ORANGE : TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(3);
    tft.setCursor(x + 20, y + 10);
    tft.print(acciones[i]);
  }
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
  tft.setCursor(200, 270); tft.printf("Vol: %d", volumen);
}

void actualizarPantallaMusica() {
  dibujarPantallaMusica();
  DPadDir dir = detectarDireccionJoystick();
  if (dir == DPAD_LEFT) {
    musicaAccionIndex = (musicaAccionIndex - 1 + 5) % 5;
    delay(200);
  }
  if (dir == DPAD_RIGHT) {
    musicaAccionIndex = (musicaAccionIndex + 1) % 5;
    delay(200);
  }
  if (leerBoton(BTN_1)) {
    switch(musicaAccionIndex) {
      case 0: musicaStartTime = 0; break;
      case 1:
        if (estadoMusica == PLAY) {
          estadoMusica = PAUSE;
          musicaStartTime = (millis() - musicaStartTime); // store elapsed
        } else {
          estadoMusica = PLAY;
          musicaStartTime = millis() - musicaStartTime; // resume
        }
        break;
      case 2: musicaStartTime += 10000; break;
      case 3: volumen = max(volumen - 1, 0); break;
      case 4: volumen = min(volumen + 1, 10); break;
    }
    delay(200);
  }
}

// =========================================================
//                      SETUP / LOOP
// =========================================================
void setup() {
  Serial.begin(115200);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);

  tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);

  cal.deadzone = 8; dht.begin();

  WiFi.begin(ssid,password);
  tft.setTextColor(TFT_YELLOW); tft.setCursor(50,120); tft.print("Conectando WiFi...");
  while(WiFi.status()!=WL_CONNECTED){ delay(500); tft.print("."); }

  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);

  tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
  tft.setCursor(30,120); tft.print("Presiona BTN1 para iniciar calibracion");
  musicaStartTime = millis();
}

void loop() {
  if(!cal.done) procesoCalibracion();
  else {
    leerDHT11();
    obtenerFraseDia();

    // Navegación tipo menú: Si joystick está arriba y luego izquierda/derecha y presionas BTN_1, cambias de pantalla
    DPadDir dir = detectarDireccionJoystick();
    static bool dashboardDrawn = false, alarmasDrawn = false, musicaDrawn = false;
    static int lastNavIndex = -1;

    // Si joystick está arriba, puedes navegar barra con izq/der y BTN_1 cambia pantalla
    static bool barraActiva = false;
    if (dir == DPAD_UP) barraActiva = true;
    else if (dir == DPAD_DOWN) barraActiva = false;

    if (barraActiva) {
      if (dir == DPAD_LEFT) { navIndex = (navIndex - 1 + 3) % 3; delay(200);}
      if (dir == DPAD_RIGHT) { navIndex = (navIndex + 1) % 3; delay(200);}
      if (leerBoton(BTN_1)) {
        pantallaActual = (Pantalla)navIndex;
        dashboardDrawn = false; alarmasDrawn = false; musicaDrawn = false;
        delay(300);
      }
    } else {
      navIndex = pantallaActual;
    }

    switch(pantallaActual) {
      case DASHBOARD:
        if (!dashboardDrawn || lastNavIndex != navIndex) { dibujarBaseDashboard(); dashboardDrawn = true; alarmasDrawn = false; musicaDrawn = false; }
        actualizarRelojFecha();
        actualizarCalendario();
        actualizarFrase();
        actualizarDatosSensor();
        break;
      case ALARMAS:
        if (!alarmasDrawn || lastNavIndex != navIndex) { dibujarPantallaAlarmas(); alarmasDrawn = true; dashboardDrawn = false; musicaDrawn = false; }
        actualizarPantallaAlarmas();
        break;
      case MUSICA:
        if (!musicaDrawn || lastNavIndex != navIndex) { dibujarPantallaMusica(); musicaDrawn = true; dashboardDrawn = false; alarmasDrawn = false; }
        actualizarPantallaMusica();
        break;
    }
    lastNavIndex = navIndex;
    delay(100);
  }
} 