#pragma once

#include <Arduino.h>
#include <vector>

namespace overseer::feature::tank {

static constexpr size_t kMaxTankSensors = 4;
static constexpr size_t kMaxTankConfigJsonBytes = 4096;

struct TankFeatureConfig {
  bool enabled = true;
  uint32_t version = 1;
  uint32_t pollIntervalMs = 1000;
  String units = "percent";
  bool restoreLastStateOnBoot = false;
  bool clearHistoryOnBoot = true;
};

struct TankCalibrationConfig {
  String mode = "two_point";
  int32_t emptyValue = 0;
  int32_t fullValue = 0;
  int32_t minValid = 0;
  int32_t maxValid = 50000;
  bool invert = false;
};

struct TankFilterConfig {
  String mode = "ema";
  bool enabled = true;
  float emaAlpha = 0.25f;
  uint8_t sampleCount = 8;
  uint16_t sampleIntervalMs = 25;
  float deadband = 1.0f;
  float spikeThreshold = 10.0f;
};

struct TankThresholdConfig {
  float emptyPercent = 5.0f;
  float lowPercent = 15.0f;
  float mediumPercent = 50.0f;
  float highPercent = 85.0f;
  float fullPercent = 95.0f;
  float overflowPercent = 99.0f;
};

struct TankFaultConfig {
  uint32_t stuckTimeoutMs = 300000;
  float noiseLimit = 8.0f;
  float maxChangePercentPerMin = 60.0f;
  uint32_t faultHoldoffMs = 5000;
};

struct TankSensorConfig {
  String id;
  String customName;
  String type = "custom";
  bool enabled = true;
  uint8_t pin = 0;
  float capacityGallons = 0.0f;
  float usableCapacityGallons = 0.0f;
  String geometryType = "linear";
  int32_t offsetFromBottomMm = 0;
  TankCalibrationConfig calibration;
  TankFilterConfig filter;
  TankThresholdConfig thresholds;
  TankFaultConfig faults;
};

struct TankSystemConfig {
  TankFeatureConfig feature;
  std::vector<TankSensorConfig> sensors;
};

TankSystemConfig makeDefaultTankSystemConfig();
String makeDefaultSensorName(const TankSensorConfig& sensor);

}  // namespace overseer::feature::tank
