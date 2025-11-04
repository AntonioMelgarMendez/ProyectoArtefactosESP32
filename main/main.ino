// ...existing code...
#include <TFT_eSPI.h>
#include "DHT.h"
#include <WiFi.h>
#include <time.h>

#include "utils.h"
#include "joystick.h"
#include "sensores.h"
#include "dashboard.h"
#include "alarmas.h"
#include "musica.h"
#include "weather.h"
#include "news.h"

// === CREDENCIALES WIFI ===
const char* ssid = "Conectando";
const char* password = "12341234";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -6 * 3600;
const int daylightOffset_sec = 0;

// Global hardware objects
TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);

// Definiciones de estado/calibración (declaradas extern en joystick.h)
JoystickCal cal;
CalState cal_state = INIT;

// Pantalla / navegación
Pantalla pantallaActual = DASHBOARD;
int navIndex = 0;
bool barraActiva = false; // nuevo: foco en la barra de navegación
bool invertYAxis = false; // nuevo: invertir eje Y

// DHT / frase del día / timers (declarados extern en sensores.h)
float temperatura = 0, humedad = 0;
unsigned long lastDHTRead = 0;
const unsigned long DHT_READ_INTERVAL = 2000;

String fraseDia = "";
unsigned long lastFraseUpdate = 0;
const unsigned long FRASE_UPDATE_INTERVAL = 3600000; // 1 hora

// Alarmas (declaradas extern en alarmas.h)
Alarma alarmas[5];
int numAlarmas = 1;
int alarmaSeleccionada = 0;
bool mostrarModalAlarma = false;
bool modoAgregarAlarma = false;
int nuevaHora = 7, nuevoMinuto = 0;
int menuAlarmaIndex = 0;

// Musica (declaradas extern en musica.h)
unsigned long musicaStartTime = 0;
unsigned long musicaDurationSec = 180; // <-- DEFINICIÓN: duración por defecto en segundos
MusicaEstado estadoMusica = PAUSE;
int volumen = 5;
int musicaAccionIndex = 0;
String cancionActual = "Cancion Ejemplo";

float internetTempC = 0.0;
String internetDesc = "";
bool internetUpdated = false;
float internetPrecipMm = 0.0;
int internetPrecipProb = 0;
float internetWind = 0.0;
int internetWeatherCode = 0;
String newsHeadline = "";
bool newsUpdated = false;

// replace with your coords and RSS URL
const double DASH_LAT = 0.0;    // <- REEMPLAZA por tu latitud
const double DASH_LON = 0.0;    // <- REEMPLAZA por tu longitud
const char* RSS_URL = "https://news.google.com/rss?hl=en-US&gl=US&ceid=US:en";

void setup() {
  Serial.begin(115200);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);

  tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);

  cal.deadzone = 8; cal.done = false; dht.begin();

  WiFi.begin(ssid,password);
  tft.setTextColor(TFT_YELLOW); tft.setCursor(50,120); tft.print("Conectando WiFi...");
  while(WiFi.status()!=WL_CONNECTED){ delay(500); tft.print("."); }

  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);

  // first fetches (optional)
  fetch_open_meteo(DASH_LAT, DASH_LON);
  fetch_first_rss_title(RSS_URL);

  tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
  tft.setCursor(30,120); tft.print("Presiona BTN1 para iniciar calibracion");

  // inicializar alarmas array (si quieres valores por defecto)
  for (int i = 0; i < 5; ++i) alarmas[i].activa = false;
  if (numAlarmas > 0) alarmas[0] = {7,0,false};
}


void loop() {
  if(!cal.done) procesoCalibracion();
  else {
    leerDHT11();
    obtenerFraseDia();

    // periodic internet updates
    internet_open_meteo_loop(DASH_LAT, DASH_LON);
    news_rss_loop(RSS_URL);

    DPadDir dir = detectarDireccionJoystick();
    static bool dashboardDrawn = false, alarmasDrawn = false, musicaDrawn = false;
    static int lastNavIndex = -1;
    static DPadDir lastDir = DPAD_NONE;

    // mapeo robusto para "arriba"/"abajo" según posible inversión física
    DPadDir upDir = invertYAxis ? DPAD_DOWN : DPAD_UP;
    DPadDir downDir = invertYAxis ? DPAD_UP : DPAD_DOWN;

    // alternar barra con JOY_SW (pulsar)
    if (leerBoton(JOY_SW)) {
      barraActiva = !barraActiva;
      dashboardDrawn = alarmasDrawn = musicaDrawn = false;
      delay(300);
    }

    // activar/desactivar la barra solo en transiciones verticales (evita repetición)
    if (dir == upDir && lastDir != upDir && !barraActiva) {
      // mover hacia 'arriba' -> activar barra
      barraActiva = true;
      dashboardDrawn = alarmasDrawn = musicaDrawn = false;
      delay(150);
    } else if (dir == downDir && lastDir != downDir && barraActiva) {
      // mover hacia 'abajo' -> desactivar barra
      barraActiva = false;
      dashboardDrawn = alarmasDrawn = musicaDrawn = false;
      delay(150);
    }

    // cuando la barra está activa: izquierda/derecha para seleccionar; BTN_1 entra; BTN_2 cancela
    if (barraActiva) {
      if (dir == DPAD_LEFT && lastDir != DPAD_LEFT) {
        navIndex = (navIndex - 1 + 3) % 3;
        dashboardDrawn = alarmasDrawn = musicaDrawn = false;
        delay(150);
      }
      if (dir == DPAD_RIGHT && lastDir != DPAD_RIGHT) {
        navIndex = (navIndex + 1) % 3;
        dashboardDrawn = alarmasDrawn = musicaDrawn = false;
        delay(150);
      }
      if (leerBoton(BTN_1)) {
        pantallaActual = (Pantalla)navIndex;
        dashboardDrawn = alarmasDrawn = musicaDrawn = false;
        barraActiva = false; // entrar a la pantalla y quitar foco de la barra
        delay(300);
      }
      if (leerBoton(BTN_2)) {
        // cancelar navegación en la barra sin cambiar pantalla
        barraActiva = false;
        dashboardDrawn = alarmasDrawn = musicaDrawn = false;
        delay(200);
      }
    } else {
      // modo pantalla: sincronizar índice de barra con la pantalla actual
      navIndex = pantallaActual;
    }

    switch(pantallaActual) {
      case DASHBOARD:
        if (!dashboardDrawn || lastNavIndex != navIndex) { dibujarBaseDashboard(navIndex); dashboardDrawn = true; alarmasDrawn = musicaDrawn = false; }
        actualizarRelojFecha();
        actualizarCalendario();
        actualizarFrase();
        actualizarDatosSensor();
        break;
      case ALARMAS:
        if (!alarmasDrawn || lastNavIndex != navIndex) { dibujarPantallaAlarmas(navIndex); alarmasDrawn = true; dashboardDrawn = musicaDrawn = false; }
        actualizarPantallaAlarmas();
        break;
      case MUSICA:
        if (!musicaDrawn || lastNavIndex != navIndex) { dibujarPantallaMusica(navIndex); musicaDrawn = true; dashboardDrawn = alarmasDrawn = false; }
        actualizarPantallaMusica();
        break;
    }

    lastNavIndex = navIndex;
    lastDir = dir;
    delay(100);
  }
} 
// ...existing code...