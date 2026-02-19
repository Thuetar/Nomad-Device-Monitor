#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "mcu_pin_map.hpp"
#include "system_log.h"
#include "system_ota.h"
#include "system_wifi.h"
#include "wifi_secret.hpp"

const int motorPin = 0;
static Logger logMain("MAIN");
static Logger logCmd("CMD");

static SystemWifi* wifi = nullptr;
static SystemOta* ota = nullptr;
static Adafruit_AHTX0 aht;

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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  SystemLogger::Config logCfg;
  logCfg.serialEnabled = true;
  logCfg.fileEnabled = false;
  logCfg.filePath = "/log.txt";
  logCfg.maxFileBytes = 64 * 1024;
  logCfg.timestamps = true;
  SystemLogger::instance().begin(logCfg);
  SystemLogger::instance().setLevel(SystemLogger::DEBUG);

  //pinMode(motorPin, OUTPUT);
  Wire.begin(i2cSdaPin, i2cSclPin);
  Wire.setClock(50000);
  Wire.setTimeOut(100);
  delay(200);
  scanI2C();
  delay(50);
  if (!aht.begin()) {
    logMain.error("AHT10 init failed");
  }

  SystemWifi::Config wifiCfg;
  wifiCfg.ssid = kWifiSsid;
  wifiCfg.password = kWifiPass;
  wifi = new SystemWifi(wifiCfg);
  wifi->setHostname("c3-mini");
  wifi->begin();  
  logMain.info(wifi->ipString());

  SystemOta::Config otaCfg;
  otaCfg.hostname = "c3-mini";
  ota = new SystemOta(otaCfg);
  ota->begin();

  logMain.info("setup complete");
}

void loop() {
  handleSerialCommands();
  if (wifi) {
    wifi->handle();
  }
  if (ota) {
    ota->handle();
  }

  static unsigned long lastStep = 0;
  if (millis() - lastStep >= 15) {
    lastStep = millis();


    sensors_event_t humidity, temp;
    if (aht.getEvent(&humidity, &temp)) {
      logMain.debug("Temp: %.2f C, RH: %.2f %%", temp.temperature, humidity.relative_humidity);
    } 
    
  }


}
