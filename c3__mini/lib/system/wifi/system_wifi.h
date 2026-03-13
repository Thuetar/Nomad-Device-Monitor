#pragma once

#include <Arduino.h>
#include <WiFi.h>

class SystemWifi {
 public:
  struct Config {
    const char* ssid = nullptr;
    const char* password = nullptr;
    bool autoReconnect = true;
    bool useDhcp = true;
    IPAddress localIp;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
  };

  explicit SystemWifi(const Config& cfg);

  void begin();
  void handle();
  bool isConnected() const;
  IPAddress ip() const;
  const char* ipString() const;
  void disconnect(bool wipe = false);

  void setHostname(const char* hostname);
  void setConnectTimeoutMs(uint32_t ms);

 private:
  void applyNetworkConfig_();
  void connect_();

  Config cfg_;
  const char* hostname_ = nullptr;
  uint32_t connectTimeoutMs_ = 15000;
  uint32_t retryIntervalMs_ = 5000;
  unsigned long lastAttemptMs_ = 0;
  mutable char ipStr_[16] = {0};
};
