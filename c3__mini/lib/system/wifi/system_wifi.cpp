#include "system_wifi.h"

SystemWifi::SystemWifi(const Config& cfg) : cfg_(cfg) {}

void SystemWifi::setHostname(const char* hostname) {
  hostname_ = hostname;
}

void SystemWifi::setConnectTimeoutMs(uint32_t ms) {
  connectTimeoutMs_ = ms;
}

void SystemWifi::applyNetworkConfig_() {
  if (cfg_.useDhcp) {
    return;
  }

  if (cfg_.dns1 == IPAddress()) {
    cfg_.dns1 = cfg_.gateway;
  }
  if (cfg_.dns2 == IPAddress()) {
    WiFi.config(cfg_.localIp, cfg_.gateway, cfg_.subnet, cfg_.dns1);
  } else {
    WiFi.config(cfg_.localIp, cfg_.gateway, cfg_.subnet, cfg_.dns1, cfg_.dns2);
  }
}

void SystemWifi::connect_() {
  if (!cfg_.ssid || cfg_.ssid[0] == '\0') {
    return;
  }

  Serial.println("SystemWifi: connect start");
  WiFi.mode(WIFI_STA);
  if (hostname_ && hostname_[0] != '\0') {
    WiFi.setHostname(hostname_);
  }
  WiFi.setAutoReconnect(cfg_.autoReconnect);
  applyNetworkConfig_();

  Serial.println("SystemWifi: calling WiFi.begin");
  WiFi.begin(cfg_.ssid, cfg_.password);
  Serial.println("SystemWifi: WiFi.begin returned");
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < connectTimeoutMs_) {
    delay(100);
  }
  Serial.println("SystemWifi: WiFi.connect returned");
}

void SystemWifi::begin() {
  connect_();
  lastAttemptMs_ = millis();
}

void SystemWifi::handle() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if ((millis() - lastAttemptMs_) < retryIntervalMs_) {
    return;
  }

  connect_();
  lastAttemptMs_ = millis();
}

bool SystemWifi::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress SystemWifi::ip() const {
  return WiFi.localIP();
}

const char* SystemWifi::ipString() const {
  IPAddress addr = WiFi.localIP();
  snprintf(ipStr_, sizeof(ipStr_), "%u.%u.%u.%u",
           addr[0], addr[1], addr[2], addr[3]);
  return ipStr_;
}

void SystemWifi::disconnect(bool wipe) {
  WiFi.disconnect(wipe);
}
