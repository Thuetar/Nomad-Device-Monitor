#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "tank/tank_config.hpp"

namespace overseer::feature::tank {

enum class SensorStatus : uint8_t {
  Ok,
  Fault,
  Offline,
  Uncalibrated
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

class TankMonitor {
public:
  void applyConfig(const TankSystemConfig& config) {
    config_ = config;
    states_.assign(config.sensors.size(), TankSensorState{});
  }

  void service() {}
  void forceRead() {}

  void clearHistory(size_t index) {
    if (index < states_.size()) {
      states_[index].history = TankHistory{};
    }
  }

  void clearAllHistory() {
    for (TankSensorState& state : states_) {
      state.history = TankHistory{};
    }
  }

  const TankSystemConfig& config() const { return config_; }
  const std::vector<TankSensorState>& states() const { return states_; }
  size_t healthySensorCount() const { return 0; }
  size_t sensorCount() const { return config_.sensors.size(); }
  uint32_t lastPollMs() const { return 0; }

private:
  TankSystemConfig config_;
  std::vector<TankSensorState> states_;
};

inline const char* toString(SensorStatus value) {
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
