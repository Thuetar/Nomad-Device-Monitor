#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ESPAsyncWebServer.h>
#include "mcu_pin_map.hpp"
#include "system_log.h"
#include "system_ota.h"
#include "system_wifi.h"
#include "wifi_secret.hpp"
#include "system_webpage_home.hpp"

static Logger logMain("MAIN");
static Logger logCmd("CMD");

static SystemWifi* wifi = nullptr;
static SystemOta* ota = nullptr;
static AsyncWebServer* webServer = nullptr;
static Adafruit_AHTX0 aht;

static int SerialPrintWait = 99000; // TODO: move to preferences... It's the ms between updates

struct TelemetrySnapshot {
  float ampVoltageV = 0.0f;
  float currentA = 0.0f;
  float tempC = 0.0f;
  float humidityPct = 0.0f;
  bool ahtValid = false;
  unsigned long lastSampleMs = 0;
  uint32_t sampleCount = 0;
  float mainLoopBusyPct = 0.0f;
};

static TelemetrySnapshot gTelemetry;

void initialize_system_ota();
void initialize_networking();
void initialize_system_log();
void initialize_web_server();
void update_measurements();
void update_main_loop_load_estimate(unsigned long loopStartUs);


static void scanI2C() {
  Serial.println("I2C scan start");
  int found = 0;
  for (uint8_t addr = 1; addr < 0x7F; ++addr) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.print("I2C device @ 0x");
      if (addr < 16) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);
      ++found;
    }
  }
  if (found == 0) {
    Serial.println("I2C scan: no devices");
  }
  Serial.println("I2C scan done");
}

static void handleSerialCommands() {
  static String line;
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (line.length() == 0) {
        continue;
      }
      line.trim();
      if (line.startsWith("log ")) {
        String level = line.substring(4);
        level.toUpperCase();
        if (level == "TRACE") {
          SystemLogger::instance().setLevel(SystemLogger::TRACE);
        } else if (level == "DEBUG") {
          SystemLogger::instance().setLevel(SystemLogger::DEBUG);
        } else if (level == "INFO") {
          SystemLogger::instance().setLevel(SystemLogger::INFO);
        } else if (level == "WARN") {
          SystemLogger::instance().setLevel(SystemLogger::WARN);
        } else if (level == "ERROR") {
          SystemLogger::instance().setLevel(SystemLogger::ERROR);
        } else if (level == "FATAL") {
          SystemLogger::instance().setLevel(SystemLogger::FATAL);
        } else {
          logCmd.warn("unknown level: %s", level.c_str());
        }
        logCmd.info("log level now %s", level.c_str());
      } else if (line == "log?") {
        logCmd.info("levels: TRACE DEBUG INFO WARN ERROR FATAL");
      } else {
        logCmd.warn("unknown cmd: %s", line.c_str());
      }
      line = "";
    } else {
      line += c;
    }
  }
}

static void initialize_mcu_pins() {
  pinMode(AMP_SENSE_PIN, INPUT);
  analogReadResolution(12);
    #if defined(ESP32)
      analogSetPinAttenuation(AMP_SENSE_PIN, ADC_11db);
    #endif
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  initialize_system_log();

  
  Wire.begin(i2cSdaPin, i2cSclPin);
  Wire.setClock(50000);
  Wire.setTimeOut(100);
  
  delay(200);
  scanI2C();
  delay(50);
  if (!aht.begin()) {
    logMain.error("AHT10 init failed");
  }

  initialize_networking();

  logMain.info(wifi->ipString());
  initialize_web_server();

  initialize_system_ota();

  logMain.info("setup complete");
}

void initialize_system_ota()
{
    SystemOta::Config otaCfg;
    otaCfg.hostname = "c3-mini";
    ota = new SystemOta(otaCfg);
    ota->begin();
}

void initialize_networking()
{
    SystemWifi::Config wifiCfg;
    wifiCfg.ssid = kWifiSsid;
    wifiCfg.password = kWifiPass;
    wifi = new SystemWifi(wifiCfg);
    wifi->setHostname("c3-mini");
    wifi->begin();
}

void initialize_system_log()
{
    SystemLogger::Config logCfg;
    logCfg.serialEnabled = true;
    logCfg.fileEnabled = false;
    logCfg.filePath = "/log.txt";
    logCfg.maxFileBytes = 64 * 1024;
    logCfg.timestamps = true;
    SystemLogger::instance().begin(logCfg);
    SystemLogger::instance().setLevel(SystemLogger::DEBUG);
}

void initialize_web_server()
{
    webServer = new AsyncWebServer(80);

    webServer->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/html; charset=utf-8", kHomePageHtml);
    });

    webServer->on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
      const uint32_t heapTotal = ESP.getHeapSize();
      const uint32_t heapFree = ESP.getFreeHeap();
      const uint32_t heapUsed = (heapTotal >= heapFree) ? (heapTotal - heapFree) : 0;
      const uint32_t heapMinFree = ESP.getMinFreeHeap();
      const uint32_t heapMaxAlloc = ESP.getMaxAllocHeap();
      const uint32_t sketchUsed = ESP.getSketchSize();
      const uint32_t sketchFreeSpace = ESP.getFreeSketchSpace();
      const uint32_t sketchTotal = sketchUsed + sketchFreeSpace;
      const float heapUsedPct = (heapTotal > 0) ? (100.0f * (float)heapUsed / (float)heapTotal) : 0.0f;
      const float sketchUsedPct = (sketchTotal > 0) ? (100.0f * (float)sketchUsed / (float)sketchTotal) : 0.0f;

      AsyncResponseStream* response = request->beginResponseStream("application/json");
      response->printf("{");
      response->printf("\"wifi_connected\":%s,", (wifi && wifi->isConnected()) ? "true" : "false");
      response->printf("\"ip\":\"%s\",", (wifi && wifi->isConnected()) ? wifi->ipString() : "0.0.0.0");
      response->printf("\"amp_voltage_v\":%.5f,", gTelemetry.ampVoltageV);
      response->printf("\"current_a\":%.5f,", gTelemetry.currentA);
      response->printf("\"aht_valid\":%s,", gTelemetry.ahtValid ? "true" : "false");
      response->printf("\"temp_c\":%.3f,", gTelemetry.tempC);
      response->printf("\"humidity_pct\":%.3f,", gTelemetry.humidityPct);
      response->printf("\"sample_count\":%lu,", (unsigned long)gTelemetry.sampleCount);
      response->printf("\"last_sample_ms\":%lu,", gTelemetry.lastSampleMs);
      response->printf("\"uptime_s\":%lu,", millis() / 1000UL);
      response->printf("\"cpu_mhz\":%u,", ESP.getCpuFreqMHz());
      response->printf("\"main_loop_busy_pct\":%.2f,", gTelemetry.mainLoopBusyPct);
      response->printf("\"heap_total\":%lu,", (unsigned long)heapTotal);
      response->printf("\"heap_used\":%lu,", (unsigned long)heapUsed);
      response->printf("\"heap_used_pct\":%.2f,", heapUsedPct);
      response->printf("\"heap_min_free\":%lu,", (unsigned long)heapMinFree);
      response->printf("\"heap_max_alloc\":%lu,", (unsigned long)heapMaxAlloc);
      response->printf("\"sketch_used\":%lu,", (unsigned long)sketchUsed);
      response->printf("\"sketch_total\":%lu,", (unsigned long)sketchTotal);
      response->printf("\"sketch_used_pct\":%.2f", sketchUsedPct);
      response->printf("}");
      request->send(response);
    });

    webServer->begin();
    logMain.info("web server started on port 80");
}

void get_ampReading() {
  float v = readVoltageAveraged(300);
  float currentA = ((v - v0) * 1000.0f) / mv_per_a;
  gTelemetry.ampVoltageV = v;
  gTelemetry.currentA = currentA;

  Serial.print("Vout=");
  Serial.print(v, 4);
  Serial.print(" V  |  I=");
  Serial.print(currentA, 3);
  Serial.println(" A");
}

float readVoltageAveraged(int samples = 200) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(AMP_SENSE_PIN);
    delayMicroseconds(200);
  }
  float raw = (float)sum / (float)samples;
  return (raw * VCC) / (float)ADC_MAX;
}

void update_measurements() {
  get_ampReading();

  sensors_event_t humidity, temp;
  if (aht.getEvent(&humidity, &temp)) {
    gTelemetry.ahtValid = true;
    gTelemetry.tempC = temp.temperature;
    gTelemetry.humidityPct = humidity.relative_humidity;
    logMain.debug("Temp: %.2f C, RH: %.2f %%", temp.temperature, humidity.relative_humidity);
  } else {
    gTelemetry.ahtValid = false;
  }

  gTelemetry.lastSampleMs = millis();
  gTelemetry.sampleCount++;
}

void update_main_loop_load_estimate(unsigned long loopStartUs) {
  static unsigned long lastLoopStartUs = 0;
  static unsigned long windowStartUs = 0;
  static uint64_t accumElapsedUs = 0;
  static uint64_t accumBusyUs = 0;

  const unsigned long loopEndUs = micros();
  const unsigned long busyUs = loopEndUs - loopStartUs;

  if (lastLoopStartUs != 0) {
    accumElapsedUs += (uint32_t)(loopStartUs - lastLoopStartUs);
  }
  accumBusyUs += busyUs;

  if (windowStartUs == 0) {
    windowStartUs = loopStartUs;
  }

  if ((uint32_t)(loopEndUs - windowStartUs) >= 5000000UL && accumElapsedUs > 0) {
    float pct = (100.0f * (float)accumBusyUs) / (float)accumElapsedUs;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    gTelemetry.mainLoopBusyPct = pct;
    accumElapsedUs = 0;
    accumBusyUs = 0;
    windowStartUs = loopStartUs;
  }

  lastLoopStartUs = loopStartUs;
}

void loop() {
  const unsigned long loopStartUs = micros();
  handleSerialCommands();
  if (wifi) {
    wifi->handle();
  }
  if (ota) {
    ota->handle();
  }
  static unsigned long lastStep = 0; //
  if (millis() - lastStep >= SerialPrintWait) {
    update_measurements();
    lastStep = millis();    //finally update 
  }
  update_main_loop_load_estimate(loopStartUs);
}
