#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "mcu_pin_map.hpp"
#include "system_log.h"
#include "system_ota.h"
#include "system_wifi.h"
#include "wifi_secret.hpp"

static Logger logMain("MAIN");
static Logger logCmd("CMD");

static SystemWifi* wifi = nullptr;
static SystemOta* ota = nullptr;
static Adafruit_AHTX0 aht;

static int SerialPrintWait = 2000; // TODO: move to preferences... It's the ms between updates

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
void get_ampReading() {
  float v = readVoltageAveraged(300);

  // Convert to current using calibrated slope:
  float currentA = ((v - v0) * 1000.0f) / mv_per_a;

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

void loop() {
  handleSerialCommands();
  if (wifi) {
    wifi->handle();
  }
  if (ota) {
    ota->handle();
  }
  static unsigned long lastStep = 0; //
  if (millis() - lastStep >= SerialPrintWait) {
    //TODO Get WCS Reading!
    get_ampReading();

    sensors_event_t humidity, temp;
    if (aht.getEvent(&humidity, &temp)) {
      logMain.debug("Temp: %.2f C, RH: %.2f %%", temp.temperature, humidity.relative_humidity);
    } 
    
    lastStep = millis();    //finally update 
  }


}
