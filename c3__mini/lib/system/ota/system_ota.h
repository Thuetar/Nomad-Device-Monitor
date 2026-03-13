#pragma once

#include <Arduino.h>

class SystemOta {
 public:
  struct Config {
    const char* hostname = nullptr;
    const char* password = nullptr;
    uint16_t port = 3232;
    bool rebootOnSuccess = true;
  };

  explicit SystemOta(const Config& cfg);

  void begin();
  void handle();
  bool isReady() const;

 private:
  Config cfg_;
  bool begun_ = false;
};
