#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ESPAsyncWebServer.h>
#include "mcu_pin_map.hpp"
#include "app.h"
#include <Preferences.h>

void initialize_mcu_pins() {
  //pinMode(AMP_SENSE_PIN, INPUT);
  pinMode(WATER_LEVEL_SENSE_PIN, INPUT);  
  // TODO: set the resolution via prefs? analogReadResolution(12)
  analogReadResolution(12);
    #if defined(ESP32)
      analogSetPinAttenuation(AMP_SENSE_PIN, ADC_11db);
      //analogSetPinAttenuation(WATER_LEVEL_SENSE_PIN, ADC_11db);
    #endif
  
}

static inline void printAppFlagsStatusJson()
{
  Serial.printf("\"show_status\":%s,", gAppFlags.showStatus ? "true" : "false");
}

void initialize_system_ota()
{
    SystemOta::Config otaCfg;
    otaCfg.hostname = "c3-mini"; //TODO: move string hostname to system preferences
    ota = new SystemOta(otaCfg);
    ota->begin();
}

void initialize_networking()
{
    SystemWifi::Config wifiCfg;
    wifiCfg.ssid = kWifiSsid;
    wifiCfg.password = kWifiPass;
    wifi = new SystemWifi(wifiCfg);
    wifi->setHostname("c3-mini"); //TODO: move string hostname to system preferences
    wifi->begin();
    //while (wifi->isConnected() == false) {
    //  Serial.println("networking ... connecting ...");
    //  delayMicroseconds(500);
    //}
    logMain.info("%s", wifi->ipString());
}

void initialize_system_log()
{
    SystemLogger::Config logCfg;
    logCfg.serialEnabled = true;    //TODO: Move to System Preferences
    logCfg.fileEnabled = false;     //TODO: Move to System Preferences
    logCfg.filePath = "/log.txt";   //TODO: Move to System Preferences
    logCfg.maxFileBytes = 64 * 1024;  //TODO: Move to System Preferences
    logCfg.timestamps = true;   //TODO: Move to System Preferences
    SystemLogger::instance().begin(logCfg);
    SystemLogger::instance().setLevel(SystemLogger::DEBUG); //TODO: Move to System Preferences
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
      response->printf("\"tank_level_raw\":%d,", gTelemetry.tankLevelRaw);
      response->printf("\"tank_level_valid\":%s,", gTelemetry.tankLevelValid ? "true" : "false");
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
    Serial.println("web server started on port 80");
}

void serialCommandMonitorTask(void *pvParameters)
{
  (void)pvParameters;
  for (;;) {
    serialCommandMonitor();
    vTaskDelay(1);
  }
}

void serialCommandMonitor() {
  int8_t cmd;
  long value = 0;
  if (ttycli.getLine()) {  // read a line of text
#if 0
    if (ttycli.eol()) {
      ttycli.reset();
      return;
    }
#endif
    static const char cmd_strings[] PROGMEM =
      "help ? start stop panic status read system_purge";
    enum {
      CMD_HELP, CMD_HELP2, START, STOP, PANIC, CMD_STATUS, CMD_READ, SYSTEM_PURGE
    };

    cmd = ttycli.keyword(cmd_strings); // look for a command.

    switch (cmd) {
      case CMD_HELP2:
      case CMD_HELP:
        Serial.println(F("Commands:"));
        Serial.println(reinterpret_cast<const __FlashStringHelper *>(cmd_strings));
        break;
      
      case START:
        Serial.print(F("NODE::START"));
        gSerialPrintWaitMs = 500;
        break;

      case STOP:
        Serial.print(F("NODE::STOP"));
        gSerialPrintWaitMs = 5000;
        break;
      
      case PANIC:
        Serial.print(F("NODE::PANIC"));
        break;

      case CMD_STATUS:
      {        
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
        Serial.print('{');
        printAppFlagsStatusJson();

        Serial.printf(
          "\"wifi_connected\":%s,\"ip\":\"%s\",\"amp_voltage_v\":%.5f,\"current_a\":%.5f,"
          "\"aht_valid\":%s,\"temp_c\":%.3f,\"humidity_pct\":%.3f,\"tank_level_raw\":%d,"
          "\"tank_level_valid\":%s,\"sample_count\":%lu,"
          "\"last_sample_ms\":%lu,\"uptime_s\":%lu,\"cpu_mhz\":%u,\"main_loop_busy_pct\":%.2f,"
          "\"heap_total\":%lu,\"heap_used\":%lu,\"heap_used_pct\":%.2f,\"heap_min_free\":%lu,"
          "\"heap_max_alloc\":%lu,\"sketch_used\":%lu,\"sketch_total\":%lu,\"sketch_used_pct\":%.2f}\n",
          (wifi && wifi->isConnected()) ? "true" : "false",
          (wifi && wifi->isConnected()) ? wifi->ipString() : "0.0.0.0",
          gTelemetry.ampVoltageV,
          gTelemetry.currentA,
          gTelemetry.ahtValid ? "true" : "false",
          gTelemetry.tempC,
          gTelemetry.humidityPct,
          gTelemetry.tankLevelRaw,
          gTelemetry.tankLevelValid ? "true" : "false",
          (unsigned long)gTelemetry.sampleCount,
          gTelemetry.lastSampleMs,
          millis() / 1000UL,
          ESP.getCpuFreqMHz(),
          gTelemetry.mainLoopBusyPct,
          (unsigned long)heapTotal,
          (unsigned long)heapUsed,
          heapUsedPct,
          (unsigned long)heapMinFree,
          (unsigned long)heapMaxAlloc,
          (unsigned long)sketchUsed,
          (unsigned long)sketchTotal,
          sketchUsedPct
        );
        
      }
      break;

      case CMD_READ: {
        Serial.print(F("UNIMPLEMENTED: "));
        break;
      }
      case SYSTEM_PURGE: {
        Serial.print(F("UNIMPLEMENTED: "));
        Serial.println(F("SYSTEM::PURGE"));
        break;
      }
      case PARSER_AMB:
        Serial.println(F("Ambiguous Command:"));
        break;

      case PARSER_EOL:
        break;

      default:
        Serial.print(F("Invalid command: "));
        Serial.println(cmd);
        break;
    }

    Serial.print(F("\nMeow> "));
    ttycli.reset();
  }
}

void handleSerialCommands() { //TODO: refactor this block into a command within the thread...
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

void scanI2C() {
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

int read_tank_sensor(int pin)
{    
  pinMode(WATER_LEVEL_SENSE_PIN, OUTPUT);
  digitalWrite(pin, LOW);
  delay(10);
  pinMode(pin, INPUT);

  const uint32_t start = micros();
  while (digitalRead(pin) == LOW) {
    if ((micros() - start) > 50000UL) {
      return -1;
    }
    delayMicroseconds(50);
  }

  return (micros() - start);
}

void initialize_system_preferences() {
  Preferences preferences;
  if (preferences.begin("nomad", false)) {
    if (preferences.isKey("update_ms") == false) {
      preferences.putULong("update_ms", kDefaultSerialPrintWaitMs);
    }
    preferences.end();
  }
} 

void loadSamplingPreferences() {
  Preferences preferences;
  if (preferences.begin("nomad", false)) {
    const uint32_t configured = preferences.getULong("update_ms", kDefaultSerialPrintWaitMs);
    preferences.end();
    if (configured > 0) {
      gSerialPrintWaitMs = configured;
    }
  }
}

bool verifyPreferenceKey(const char* preferenceKey)
{
  if (preferenceKey == nullptr || preferenceKey[0] == '\0') {
    return false;
  }

  return false;
}
