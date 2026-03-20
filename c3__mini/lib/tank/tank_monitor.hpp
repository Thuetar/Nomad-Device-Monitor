#pragma once

#include <Arduino.h>
#include <vector>

namespace overseer::feature::tank {

enum class SensorType : uint8_t {
  Fresh,
  Gray,
  Black,
  Custom
};

enum class SensorStatus : uint8_t {
  Ok,
  Fault,
  Offline,
  Uncalibrated
};

struct TankCalibrationConfig {
  int32_t emptyValue = 0;
  int32_t fullValue = 0;
  int32_t minValid = 0;
  int32_t maxValid = 4095;
  bool invert = false;
};

struct TankFilterConfig {
  bool enabled = true;
  float emaAlpha = 0.25f;
};

struct TankSensorConfig {
  String id;
  String customName;
  SensorType type = SensorType::Custom;
  bool enabled = true;
  uint8_t pin = 0;
  float capacityGallons = 0.0f;
  TankCalibrationConfig calibration;
  TankFilterConfig filter;
};

struct TankReading {
  uint32_t lastReadMs = 0;
  int32_t raw = -1;
  float filtered = 0.0f;
  float percent = 0.0f;
  float volumeGallons = 0.0f;
  bool valid = false;
};

struct TankHistory {
  float lowestPercent = 0.0f;
  float highestPercent = 0.0f;
  uint32_t samplesTaken = 0;
  uint32_t readErrorCount = 0;
};

struct TankSensorState {
  SensorStatus status = SensorStatus::Offline;
  TankReading reading;
  TankHistory history;
  bool filterInitialized = false;
};

struct TankMonitorConfig {
  bool enabled = true;
  uint32_t version = 1;
  uint32_t pollIntervalMs = 1000;
  String units = "percent";
  std::vector<TankSensorConfig> sensors;
};

class TankMonitor {
public:
  TankMonitor();

  void begin();
  void service();
  void forceRead();

  const TankMonitorConfig& config() const { return config_; }
  const std::vector<TankSensorState>& states() const { return states_; }

  bool isEnabled() const { return config_.enabled; }
  size_t sensorCount() const { return config_.sensors.size(); }
  size_t healthySensorCount() const;
  uint32_t lastPollMs() const { return lastPollMs_; }

private:
  void loadDefaults();
  void loadConfig();
  void syncTelemetrySummary();
  void readAllSensors();
  void readSensor(size_t index);
  int32_t readRawSensor(uint8_t pin);
  bool isCalibrationValid(const TankCalibrationConfig& calibration) const;
  bool isRawValid(int32_t raw, const TankCalibrationConfig& calibration) const;
  float scalePercent(int32_t raw, const TankCalibrationConfig& calibration) const;
  float applyFilter(size_t index, float inputPercent);
  float computeVolumeGallons(float percent, float capacityGallons) const;
  void updateHistory(TankSensorState& state);

  TankMonitorConfig config_;
  std::vector<TankSensorState> states_;
  uint32_t lastPollMs_ = 0;
};

const char* toString(SensorType value);
const char* toString(SensorStatus value);

}  // namespace overseer::feature::tank
