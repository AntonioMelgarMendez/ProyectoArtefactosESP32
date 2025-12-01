#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <vector>
#include <WiFi.h>
#include <time.h>
#include "DHT.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- AUDIO ---
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "BluetoothA2DPSink.h"
#include "ESP_I2S.h" 

// --- BLUETOOTH SERIAL ---
#include "BluetoothSerial.h"

#include "utils.h"
#include "joystick.h"
#include "sensores.h" 
#include "dashboard.h"
#include "alarmas.h"
#include "musica.h"
#include "weather.h"
#include "news.h"

RTC_NOINIT_ATTR int modoArranque; 

// OBJETOS GLOBALES
I2SClass i2s; 
BluetoothA2DPSink *a2dp_sink = NULL;
BluetoothSerial SerialBT;

AudioGeneratorMP3 *mp3 = NULL;
AudioFileSourceSD *file = NULL;
AudioOutputI2S *out = NULL;

AudioMode currentAudioMode; 
String btTitle = "";
String btArtist = "";
String btAlbum = "";
bool btMetadataChanged = false;

// --- CALLBACK METADATA ---
void avrc_metadata_callback(uint8_t attr_id, const uint8_t *value) {
  switch(attr_id) {
    case ESP_AVRC_MD_ATTR_TITLE:  btTitle = (const char*)value; break;
    case ESP_AVRC_MD_ATTR_ARTIST: btArtist = (const char*)value; break;
    case ESP_AVRC_MD_ATTR_ALBUM:  btAlbum = (const char*)value; break;
  }
  btMetadataChanged = true; 
}

const char* ssid = "TIGO-4B5C";
const char* password = "4D99ED801413";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -6 * 3600;
const int daylightOffset_sec = 0;

TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);

#define SD_SCK 22
#define SD_MISO 21
#define SD_MOSI 13
#define SD_CS_PIN 5
#define I2S_BCK 14
#define I2S_LRC 27
#define I2S_DOUT 26
#define PIN_AMP_ENABLE 12

SPIClass sdSPI(HSPI); 
std::vector<String> playlist;
int songIndex = 0;
bool isPaused = false;
float currentVolume = 0.3; 
const char* folderPath = "/80s Gangsta Rap"; 

JoystickCal cal;
CalState cal_state = INIT;
Pantalla pantallaActual = DASHBOARD;
int navIndex = 0;
bool barraActiva = false; 
bool invertYAxis = false; 

const unsigned long NAV_DELAY = 150;
const unsigned long BTN_DELAY = 300;

volatile float temperatura = 0;
volatile float humedad = 0;
unsigned long lastDHTRead = 0;

String fraseDia = "";
unsigned long lastFraseUpdate = 0;
const unsigned long FRASE_UPDATE_INTERVAL = 3600000; 
Alarma alarmas[5];
int numAlarmas = 1;
int alarmaSeleccionada = 0;
bool mostrarModalAlarma = false;
bool modoAgregarAlarma = false;
int nuevaHora = 7, nuevoMinuto = 0;
int menuAlarmaIndex = 0;
unsigned long musicaStartTime = 0; 
unsigned long musicaDurationSec = 180; 
int volumen = 5; 
String cancionActual = "Cargando...";
float internetTempC = 0.0;
String internetDesc = "";
bool internetUpdated = false;
float internetPrecipMm = 0.0;
int internetPrecipProb = 0;
float internetWind = 0.0;
int internetWeatherCode = 0;
String newsHeadline = "";
bool newsUpdated = false;
const double DASH_LAT = 13.6929;    
const double DASH_LON = -89.2182;
const char* RSS_URL = "https://news.google.com/rss?hl=en-US&gl=US&ceid=US:en";

TaskHandle_t TaskSensoresHandle;

// -------------------------------------------------------------------------
// UI & CARGA
// -------------------------------------------------------------------------
void dibujarBarraProgreso(int porcentaje, String mensaje) {
  int w = 300; int h = 20; int x = (480 - w) / 2; int y = 260;
  tft.drawRect(x, y, w, h, TFT_WHITE);
  int fillW = (w - 4) * porcentaje / 100;
  tft.fillRect(x + 2, y + 2, fillW, h - 4, TFT_GREEN);
  tft.fillRect(0, y + 30, 480, 30, TFT_BLACK); 
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(mensaje, 240, y + 45, 2);
}

void mostrarPantallaCarga() {
  tft.fillScreen(TFT_BLACK); tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.drawString("POWER BY ESP32", 240, 100, 4); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawString("JEFRY BOT", 240, 140, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); tft.drawString("Iniciando sistema...", 240, 180, 2);
}

void cargarPlaylist(String path) {
  File root = SD.open(path);
  if(!root || !root.isDirectory()) return;
  File f = root.openNextFile();
  while(f){ if(!f.isDirectory()) { String n = String(f.name()); if(n.endsWith(".mp3") || n.endsWith(".MP3")) playlist.push_back(n); } f = root.openNextFile(); }
}

void playCurrentSong(bool autoPlay) {
  if (currentAudioMode != MODE_SD || !mp3) return;
  if (mp3->isRunning()) mp3->stop();
  if (file) { delete file; file = NULL; }
  if (autoPlay) { isPaused = false; digitalWrite(PIN_AMP_ENABLE, HIGH); if(out) out->SetGain(currentVolume); } 
  else { isPaused = true; digitalWrite(PIN_AMP_ENABLE, LOW); if(out) out->SetGain(0); }
  if (playlist.size() > 0) {
    String fileName = playlist[songIndex]; String fullPath = String(folderPath) + "/" + fileName;
    file = new AudioFileSourceSD(fullPath.c_str());
    if(mp3) if(!mp3->begin(file, out)) { delay(100); songIndex = (songIndex + 1) % playlist.size(); playCurrentSong(true); }
  }
}

void procesarComandosBT() {
  if (SerialBT.available()) {
    String comando = SerialBT.readStringUntil('\n'); comando.trim();
    if(a2dp_sink != NULL && currentAudioMode == MODE_BT) {
        if (comando == "play") a2dp_sink->play();
        else if (comando == "pause") a2dp_sink->pause();
        else if (comando == "next") a2dp_sink->next();
        else if (comando == "prev") a2dp_sink->previous();
        else if (comando == "vol+") { currentVolume += 0.1; if(currentVolume > 1.0) currentVolume = 1.0; a2dp_sink->set_volume((uint8_t)(currentVolume * 127)); }
        else if (comando == "vol-") { currentVolume -= 0.1; if(currentVolume < 0) currentVolume = 0; a2dp_sink->set_volume((uint8_t)(currentVolume * 127)); }
    }
  }
}

// --- TAREA SEGUNDO PLANO (Core 0) ---
void TaskSensoresCode(void * pvParameters) {
  Serial.println("Tarea Sensores Iniciada (Core 0)");
  // Damos tiempo a que el setup termine
  vTaskDelay(3000 / portTICK_PERIOD_MS); 

  for(;;) { 
    if (currentAudioMode == MODE_SD) {
        gestionarSensoresYEnvio(false); 
        if (WiFi.status() == WL_CONNECTED) {
             obtenerFraseDia();
             internet_open_meteo_loop(DASH_LAT, DASH_LON);
             news_rss_loop(RSS_URL);
        }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  
  pinMode(BTN_1, INPUT_PULLUP); pinMode(BTN_2, INPUT_PULLUP);
  pinMode(PIN_AMP_ENABLE, OUTPUT); digitalWrite(PIN_AMP_ENABLE, HIGH); 
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT); digitalWrite(SD_CS_PIN, HIGH); 

  tft.init(); tft.setRotation(3); mostrarPantallaCarga();
  dibujarBarraProgreso(10, "Cargando Hardware..."); delay(500);
  
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdSPI, 10000000)) { Serial.println("Fallo SD"); dibujarBarraProgreso(15, "Error SD Card!"); delay(1000); }
  else { cargarPlaylist(folderPath); dibujarBarraProgreso(30, "SD Montada OK"); }

  dht.begin(); 

  if (modoArranque != 0 && modoArranque != 1) modoArranque = 0;

  if (modoArranque == 1) {
      // MODO BLUETOOTH
      dibujarBarraProgreso(50, "Iniciando Bluetooth...");
      currentAudioMode = MODE_BT; pantallaActual = MUSICA;
      WiFi.mode(WIFI_OFF); 
      i2s.setPins(I2S_BCK, I2S_LRC, I2S_DOUT);
      if (!i2s.begin(I2S_MODE_STD, 44100, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) { Serial.println("Error I2S"); while(1); }

      a2dp_sink = new BluetoothA2DPSink(i2s);
      a2dp_sink->set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM);
      a2dp_sink->set_avrc_metadata_callback(avrc_metadata_callback);
      a2dp_sink->start("ESP32_PLAYER");
      a2dp_sink->set_volume((uint8_t)(currentVolume * 127));

      SerialBT.begin("ESP32_PLAYER"); 
      btTitle = "Esperando..."; btArtist = "Conectar Bluetooth";
      dibujarBarraProgreso(100, "Bluetooth Listo!"); delay(500);

  } else {
      // MODO SD
      currentAudioMode = MODE_SD;
      mostrarPantallaCarga();
      dibujarBarraProgreso(10, "Iniciando Audio SD...");

      out = new AudioOutputI2S(); out->SetPinout(I2S_BCK, I2S_LRC, I2S_DOUT); out->SetGain(currentVolume);
      mp3 = new AudioGeneratorMP3();
      
      dibujarBarraProgreso(30, "Conectando WiFi...");
      WiFi.begin(ssid, password);
      unsigned long startWifi = millis();
      while(WiFi.status() != WL_CONNECTED && millis() - startWifi < 6000) { 
         delay(250); dibujarBarraProgreso(35, "Conectando WiFi...");
      }
      
      if(WiFi.status() == WL_CONNECTED) {
        dibujarBarraProgreso(75, "Conectado. Sincronizando...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        internet_open_meteo_loop(DASH_LAT, DASH_LON);
        fetch_first_rss_title(RSS_URL);
        obtenerFraseDia();
      } else { dibujarBarraProgreso(80, "Modo Offline"); delay(1000); }
      
      dibujarBarraProgreso(90, "Iniciando Tareas de Fondo...");

      xTaskCreatePinnedToCore(
        TaskSensoresCode,   
        "TaskSensores",     
        30720,              
        NULL,               
        1,                  
        &TaskSensoresHandle,
        0); // Core 0
        
      dibujarBarraProgreso(100, "Sistema Listo!"); delay(500);
      if(playlist.size() > 0) playCurrentSong(false);
  }
  cal.deadzone = 200; 
  cal.done = true; 
  
  tft.fillScreen(TFT_BLACK); 
  if (numAlarmas > 0) alarmas[0] = {7, 0, false};
}

// -------------------------------------------------------------------------
// LOOP (NÚCLEO 1)
// -------------------------------------------------------------------------
void loop() {
  if (currentAudioMode == MODE_SD && mp3) {
      if (mp3->isRunning() && !isPaused) { if (!mp3->loop()) { mp3->stop(); songIndex = (songIndex + 1) % playlist.size(); playCurrentSong(true); } }
  } else { delay(5); }

  if (currentAudioMode == MODE_BT) { procesarComandosBT(); }
  
  if(!cal.done) { 

      procesoCalibracion(); 
  } 
  else {

    unsigned long now = millis();
    DPadDir dir = detectarDireccionJoystick();
    static bool dashboardDrawn = false, alarmasDrawn = false, musicaDrawn = false;
    static int lastNavIndex = -1; static DPadDir lastDir = DPAD_NONE;

    if (pantallaActual == MUSICA && !barraActiva) {
        bool inputConsumido = musicaHandleInput(dir, leerBoton(BTN_1), leerBoton(BTN_2));
        if (!inputConsumido && dir == DPAD_UP && lastDir != DPAD_UP) { barraActiva = true; delay(150); }
        lastDir = dir; 
    }
    else {
        if (now - lastNavTime > NAV_DELAY) {
          bool huboMovimiento = false;
          if (dir == DPAD_UP && lastDir != DPAD_UP && !barraActiva) { barraActiva = true; huboMovimiento = true; } 
          else if (dir == DPAD_DOWN && lastDir != DPAD_DOWN && barraActiva) { 
              barraActiva = false; huboMovimiento = true; 
              if(pantallaActual == DASHBOARD) dashboardDrawn = false;
              if(pantallaActual == ALARMAS) alarmasDrawn = false;
              if(pantallaActual == MUSICA) musicaDrawn = false;
          }
          if (barraActiva) {
            if (dir == DPAD_LEFT && lastDir != DPAD_LEFT) { navIndex = (navIndex - 1 + 3) % 3; huboMovimiento = true; }
            if (dir == DPAD_RIGHT && lastDir != DPAD_RIGHT) { navIndex = (navIndex + 1) % 3; huboMovimiento = true; }
          } else { navIndex = pantallaActual; }
          if (huboMovimiento) lastNavTime = now;
        }
        if (now - lastBtnTime > BTN_DELAY) {
          if (barraActiva) {
            if (leerBoton(BTN_1)) { pantallaActual = (Pantalla)navIndex; dashboardDrawn = false; alarmasDrawn = false; musicaDrawn = false; barraActiva = false; lastBtnTime = now; }
            if (leerBoton(BTN_2)) { barraActiva = false; if(pantallaActual == DASHBOARD) dashboardDrawn = false; if(pantallaActual == ALARMAS) alarmasDrawn = false; if(pantallaActual == MUSICA) musicaDrawn = false; lastBtnTime = now; }
          }
        }
        lastDir = dir;
    }

    switch(pantallaActual) {
      case DASHBOARD:
        { bool forceUpdate = !dashboardDrawn; if (forceUpdate) { dibujarBaseDashboard(navIndex); dashboardDrawn = true; alarmasDrawn = false; musicaDrawn = false; } dibujarBarraNavegacion(navIndex); if(currentAudioMode == MODE_SD) { actualizarRelojFecha(forceUpdate); actualizarCalendario(forceUpdate); actualizarFrase(forceUpdate); actualizarDatosSensor(forceUpdate); } } break;
      case ALARMAS:
        if (!alarmasDrawn) { dibujarPantallaAlarmas(navIndex); alarmasDrawn = true; dashboardDrawn = false; musicaDrawn = false; } dibujarBarraNavegacion(navIndex); actualizarPantallaAlarmas(); break;
      case MUSICA:
        if(playlist.size() > 0 && currentAudioMode == MODE_SD) cancionActual = playlist[songIndex];
        if (!musicaDrawn) { dibujarPantallaMusica(navIndex); musicaDrawn = true; dashboardDrawn = false; alarmasDrawn = false; } dibujarBarraNavegacion(navIndex); actualizarPantallaMusica(); break;
    }
  }
}