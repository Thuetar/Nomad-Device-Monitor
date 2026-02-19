#include "system_ota.h"

#include <ArduinoOTA.h>

SystemOta::SystemOta(const Config& cfg) : cfg_(cfg) {}

void SystemOta::begin() {
  if (cfg_.hostname && cfg_.hostname[0] != '\0') {
    ArduinoOTA.setHostname(cfg_.hostname);
  }
  if (cfg_.password && cfg_.password[0] != '\0') {
    ArduinoOTA.setPassword(cfg_.password);
  }
  if (cfg_.port != 0) {
    ArduinoOTA.setPort(cfg_.port);
  }

  ArduinoOTA
      .onStart([]() { Serial.println("OTA: start"); })
      .onEnd([]() { Serial.println("OTA: end"); })
      .onError([](ota_error_t error) {
        Serial.print("OTA: error ");
        Serial.println(error);
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        const uint8_t pct = (total == 0) ? 0 : (progress * 100U / total);
        Serial.print("OTA: ");
        Serial.print(pct);
        Serial.println("%");
      });

  ArduinoOTA.setRebootOnSuccess(cfg_.rebootOnSuccess);
  ArduinoOTA.begin();
  begun_ = true;
}

void SystemOta::handle() {
  if (!begun_) {
    return;
  }
  ArduinoOTA.handle();
}

bool SystemOta::isReady() const {
  return begun_;
}
