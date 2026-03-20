#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ESPAsyncWebServer.h>
#include "targets/board_config.h"
#include "core_system_globals.hpp"
#include "app.h"
#include "tank/tank_service.hpp"



APPLICATION_STATUS_FLAGS gAppFlags;
simpleParser<> ttycli(Serial);
TaskHandle_t serialCommandMonitor_t = nullptr;
TaskHandle_t telemetryMonitor_t = nullptr;

TelemetrySnapshot gTelemetry;
uint32_t gSerialPrintWaitMs = kDefaultSerialPrintWaitMs;
int touchpin1 = WATER_LEVEL_SENSE_PIN;
overseer::feature::tank::TankService* tankService = nullptr;

/*
TODO: only output to stdout when flags set
TODO: implement command to calibrate current sensor

*/

static constexpr TickType_t kTelemetryUpdateIntervalTicks = pdMS_TO_TICKS(1000);

static void telemetryMonitorTask(void *pvParameters) {
  (void)pvParameters;

  TickType_t lastWakeTime = xTaskGetTickCount();
  for (;;) {
    update_measurements();
    if (tankService) {
      tankService->service();
    }
    vTaskDelayUntil(&lastWakeTime, kTelemetryUpdateIntervalTicks);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Loading System Prefs... ");
  initialize_system_preferences();
  Serial.println("Loaded System Prefs.");
  loadSamplingPreferences();

  Serial.println("Initializing log...");
  initialize_system_log();
  Serial.println("Log initialized.");

  Serial.println("Initializing networking...");
  initialize_networking();
  Serial.println("Networking initialized.");
  Serial.print("IP: ");
  Serial.println(wifi ? wifi->ipString() : "0.0.0.0");
  
  Serial.println("Initializing OTA...");
  initialize_system_ota();
  Serial.println("OTA initialized.");

  Serial.println("Initializing I2C...");
  Wire.begin(i2cSdaPin, i2cSclPin);
  Wire.setClock(50000);
  Wire.setTimeOut(100);
  Serial.println("I2C initialized.");
  
  Serial.println("Initializing MCU pins...");
  initialize_mcu_pins();
  Serial.println("MCU pins initialized.");

  tankService = new overseer::feature::tank::TankService();
  tankService->begin();

  Serial.println("Initializing web server...");
  initialize_web_server();
  Serial.println("Web server initialized.");
  
  delay(200);
  Serial.println("Starting I2C scan...");
  scanI2C();
  delay(50);
  Serial.println("Initializing AHT...");
  if (!aht.begin()) {
    logMain.error("AHT10 init failed");
  }
  Serial.println("AHT init complete.");

  Serial.println("Measuring v0...");
  v0 = readVoltageAveraged(800);
  Serial.print("Auto-zero v0=");
  Serial.println(v0, 4);

  logMain.info("serialCommandMonitor_t:: Starting");
  
  xTaskCreatePinnedToCore(
    serialCommandMonitorTask,       /* Function to implement the task */
    "serialCommandMonitor_t",       /* Name of the task */
    10000,                          /* Stack size in words */
    NULL,                           /* Task input parameter */
    1,                              /* Priority of the task */
    &serialCommandMonitor_t,        /* Task handle */
    0);                             /* Core ID (0 or 1) */

  logMain.info("serialCommandMonitor_t:: setup complete");

  xTaskCreatePinnedToCore(
    telemetryMonitorTask,
    "telemetryMonitor_t",
    8192,
    NULL,
    1,
    &telemetryMonitor_t,
    0);

  logMain.info("SYSTEM INITIALIZED");
}

void loop() {
  const unsigned long loopStartUs = micros();
  
  if (wifi) {
    wifi->handle();
  }
  if (ota) {
    ota->handle();
  }
  update_main_loop_load_estimate(loopStartUs);  
  // logMain.debug("MOWING: The Read... Started.");
  // logMain.debug("Water Sensor: %d %%", read_tank_sensor(WATER_LEVEL_SENSE_PIN));
  // const unsigned long nowUs = micros();
  // pinMode(WATER_LEVEL_SENSE_PIN, OUTPUT);
  // digitalWrite(WATER_LEVEL_SENSE_PIN, LOW);
  // logMain.debug("MOWING: The Read... Now Low.");
  // delay(100);
  //Serial.println(touchRead(T2));  // get value using T2
  //delay(1000);
  //do {
  //} while (digitalRead(pin) == LOW);
}

void get_ampReading() {
  float v = readVoltageAveraged(300);
  float currentA = ((v - v0) * 1000.0f) / mv_per_a;
  v = (v - v0);
  gTelemetry.ampVoltageV = v;
  gTelemetry.currentA = currentA;

  // NewFunction(v, currentA);
}

void NewFunction(float v, float currentA)
{
    Serial.print("Vout=");
    Serial.print(v, 4);
    Serial.print(" V  |  I=");
    Serial.print(currentA, 3);
    Serial.println(" A");
}

/** @brief Low level function, gets raw data. 
 *  @attention This returns the "Automagically CORRECTED" (i.e. raw - auto-correction_reading) voltage reading.
 */
float readVoltageAveraged(int samples = 200) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(AMP_SENSE_PIN);
    delayMicroseconds(200);
  }
  float raw = (float)sum / (float)samples;
  return ( (raw * VCC) / (float)ADC_MAX );
}

void update_measurements() {
  get_ampReading();

  sensors_event_t humidity, temp;
  if (aht.getEvent(&humidity, &temp)) {
    gTelemetry.ahtValid = true;
    gTelemetry.tempC = temp.temperature;
    
    gTelemetry.humidityPct = humidity.relative_humidity;
    //logMain.debug("Temp: %.2f C, RH: %.2f %%", temp.temperature, humidity.relative_humidity);
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
