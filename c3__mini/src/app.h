//#include "system_log.h"
#include <Adafruit_AHTX0.h>
#include <ESPAsyncWebServer.h>

#include "log/system_log.h"
#include "ota/system_ota.h"
#include "wifi/system_wifi.h"
#include "system_webpage_home.hpp"
#include "wifi_secret.hpp"
#include "simpleParser.h"
#include "targets/board_config.h"

#ifndef APP_H
#define APP_H

namespace overseer::feature::tank {
class TankMonitor;
}

static constexpr uint32_t kDefaultSerialPrintWaitMs = 99000UL;
extern uint32_t gSerialPrintWaitMs;

struct TelemetrySnapshot {
  float ampVoltageV = 0.0f;
  float currentA = 0.0f;
  float tempC = 0.0f;
  float humidityPct = 0.0f;
  bool ahtValid = false;
  bool tankFeatureEnabled = false;
  unsigned long lastSampleMs = 0;
  uint32_t tankLastPollMs = 0;
  uint32_t sampleCount = 0;
  uint8_t tankSensorCount = 0;
  uint8_t tankHealthySensorCount = 0;
  float mainLoopBusyPct = 0.0f;
};
extern TelemetrySnapshot gTelemetry;
extern overseer::feature::tank::TankMonitor* tankMonitor;

extern struct APPLICATION_STATUS_FLAGS
{
  bool showStatus = false;
  bool is_Calibrated = false;

} gAppFlags;


static Logger logMain("MAIN");
static Logger logCmd("CMD");

static SystemWifi* wifi = nullptr;
static SystemOta* ota = nullptr;
static AsyncWebServer* webServer = nullptr;
static Adafruit_AHTX0 aht;

void handleSerialCommands();                  // Janky code... ai slop.
void serialCommandMonitor();                  // -- Serial Command Handler
void serialCommandMonitorTask(void *pvParameters); // -- Serial Command Handler Thread
extern TaskHandle_t serialCommandMonitor_t;    // Command and Control
extern simpleParser<> ttycli;

void loadSamplingPreferences();
bool verifyPreferenceKey(const char* preferenceKey);
void initialize_system_ota();
void initialize_networking();
void initialize_system_log();
void initialize_web_server();
void initialize_mcu_pins();
void initialize_system_preferences();
void update_measurements();
void update_main_loop_load_estimate(unsigned long loopStartUs);
void scanI2C();

#endif
