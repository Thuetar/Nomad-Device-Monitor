#include "tank/tank_monitor.hpp"

#include <algorithm>

#include "app.h"

namespace overseer::feature::tank {

namespace {
constexpr uint32_t kRawReadTimeoutUs = 50000UL;
constexpr uint32_t kRawReadSettleMs = 10UL;

float clampPercent(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 100.0f) {
    return 100.0f;
  }
  return value;
}
}  // namespace

void TankMonitor::applyConfig(const TankSystemConfig& config) {
  config_ = config;
  states_.assign(config_.sensors.size(), TankSensorState{});
  lastPollMs_ = 0;
  syncTelemetrySummary();
}

void TankMonitor::service() {
  if (!config_.feature.enabled) {
    for (TankSensorState& state : states_) {
      state.status = SensorStatus::Offline;
      state.reading.valid = false;
    }
    syncTelemetrySummary();
    return;
  }

  const uint32_t now = millis();
  if (lastPollMs_ != 0 && (now - lastPollMs_) < config_.feature.pollIntervalMs) {
    return;
  }

  readAllSensors();
  lastPollMs_ = now;
  syncTelemetrySummary();
}

void TankMonitor::forceRead() {
  lastPollMs_ = 0;
  service();
}

void TankMonitor::clearHistory(size_t index) {
  if (index >= states_.size()) {
    return;
  }
  states_[index].history = TankHistory{};
}

void TankMonitor::clearAllHistory() {
  for (TankSensorState& state : states_) {
    state.history = TankHistory{};
  }
}

size_t TankMonitor::healthySensorCount() const {
  size_t count = 0;
  for (const TankSensorState& state : states_) {
    if (state.status == SensorStatus::Ok) {
      ++count;
    }
  }
  return count;
}

void TankMonitor::syncTelemetrySummary() {
  gTelemetry.tankFeatureEnabled = config_.feature.enabled;
  gTelemetry.tankLastPollMs = lastPollMs_;
  gTelemetry.tankSensorCount = static_cast<uint8_t>(config_.sensors.size());
  gTelemetry.tankHealthySensorCount = static_cast<uint8_t>(healthySensorCount());
}

void TankMonitor::readAllSensors() {
  if (states_.size() != config_.sensors.size()) {
    states_.assign(config_.sensors.size(), TankSensorState{});
  }

  for (size_t i = 0; i < config_.sensors.size(); ++i) {
    readSensor(i);
  }
}

void TankMonitor::readSensor(size_t index) {
  if (index >= config_.sensors.size() || index >= states_.size()) {
    return;
  }

  const TankSensorConfig& cfg = config_.sensors[index];
  TankSensorState& state = states_[index];

  if (!cfg.enabled) {
    state.status = SensorStatus::Offline;
    state.reading.valid = false;
    return;
  }

  if (!isCalibrationValid(cfg.calibration)) {
    state.status = SensorStatus::Uncalibrated;
    state.reading.valid = false;
    return;
  }

  const int32_t raw = readRawSensor(cfg.pin);
  state.reading.lastReadMs = millis();
  state.reading.raw = raw;
  state.history.samplesTaken++;

  if (!isRawValid(raw, cfg.calibration)) {
    state.status = SensorStatus::Fault;
    state.reading.valid = false;
    state.history.readErrorCount++;
    return;
  }

  const float scaledPercent = scalePercent(raw, cfg.calibration);
  const float filteredPercent = applyFilter(index, scaledPercent);

  state.status = SensorStatus::Ok;
  state.reading.filtered = filteredPercent;
  state.reading.percent = filteredPercent;
  const float volumeCapacity = (cfg.usableCapacityGallons > 0.0f) ? cfg.usableCapacityGallons : cfg.capacityGallons;
  state.reading.volumeGallons = computeVolumeGallons(filteredPercent, volumeCapacity);
  state.reading.valid = true;
  updateHistory(state);
}

int32_t TankMonitor::readRawSensor(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(kRawReadSettleMs);
  pinMode(pin, INPUT);

  const uint32_t start = micros();
  while (digitalRead(pin) == LOW) {
    if ((micros() - start) > kRawReadTimeoutUs) {
      return -1;
    }
    delayMicroseconds(50);
  }

  return static_cast<int32_t>(micros() - start);
}

bool TankMonitor::isCalibrationValid(const TankCalibrationConfig& calibration) const {
  return calibration.fullValue > calibration.emptyValue;
}

bool TankMonitor::isRawValid(int32_t raw, const TankCalibrationConfig& calibration) const {
  return raw >= calibration.minValid && raw <= calibration.maxValid;
}

float TankMonitor::scalePercent(int32_t raw, const TankCalibrationConfig& calibration) const {
  const float denominator = static_cast<float>(calibration.fullValue - calibration.emptyValue);
  if (denominator <= 0.0f) {
    return 0.0f;
  }

  float percent = 100.0f * static_cast<float>(raw - calibration.emptyValue) / denominator;
  if (calibration.invert) {
    percent = 100.0f - percent;
  }
  return clampPercent(percent);
}

float TankMonitor::applyFilter(size_t index, float inputPercent) {
  if (index >= config_.sensors.size() || index >= states_.size()) {
    return inputPercent;
  }

  const TankSensorConfig& cfg = config_.sensors[index];
  TankSensorState& state = states_[index];
  if (!cfg.filter.enabled) {
    state.filterInitialized = true;
    return inputPercent;
  }

  const float alpha = std::clamp(cfg.filter.emaAlpha, 0.0f, 1.0f);
  if (!state.filterInitialized) {
    state.filterInitialized = true;
    return inputPercent;
  }

  return ((alpha * inputPercent) + ((1.0f - alpha) * state.reading.percent));
}

float TankMonitor::computeVolumeGallons(float percent, float capacityGallons) const {
  if (capacityGallons <= 0.0f) {
    return 0.0f;
  }
  return (clampPercent(percent) / 100.0f) * capacityGallons;
}

void TankMonitor::updateHistory(TankSensorState& state) {
  if (!state.reading.valid) {
    return;
  }

  if (state.history.samplesTaken <= 1 || state.reading.percent < state.history.lowestPercent) {
    state.history.lowestPercent = state.reading.percent;
  }
  if (state.history.samplesTaken <= 1 || state.reading.percent > state.history.highestPercent) {
    state.history.highestPercent = state.reading.percent;
  }
}

const char* toString(SensorStatus value) {
  switch (value) {
    case SensorStatus::Ok:
      return "ok";
    case SensorStatus::Fault:
      return "fault";
    case SensorStatus::Offline:
      return "offline";
    case SensorStatus::Uncalibrated:
    default:
      return "uncalibrated";
  }
}

}  // namespace overseer::feature::tank
