#pragma once

#include "config/config_result.hpp"
#include "tank/tank_config.hpp"
#include "tank/tank_config_store.hpp"
#include "tank/tank_monitor.hpp"

namespace overseer::feature::tank {

struct TankServiceStatus {
  bool configLoaded = false;
  bool configDegraded = false;
  String lastConfigError;
};

class TankService {
public:
  void begin();
  void service();
  void forceRead();

  const TankSystemConfig& config() const { return config_; }
  const std::vector<TankSensorState>& states() const { return monitor_.states(); }
  const TankServiceStatus& status() const { return status_; }

  ConfigResult replaceConfig(const TankSystemConfig& config);
  ConfigResult createSensor(const TankSensorConfig& sensor);
  ConfigResult updateSensor(const String& id, const TankSensorConfig& sensor);
  ConfigResult patchSensorEnabled(const String& id, bool enabled);
  ConfigResult deleteSensor(const String& id);
  ConfigResult updateCalibration(const String& id, const TankCalibrationConfig& calibration);
  ConfigResult clearHistory(const String& id);
  ConfigResult clearAllHistory();

  size_t healthySensorCount() const { return monitor_.healthySensorCount(); }
  size_t sensorCount() const { return monitor_.sensorCount(); }
  uint32_t lastPollMs() const { return monitor_.lastPollMs(); }

private:
  ConfigResult validateConfig(TankSystemConfig& config) const;
  ConfigResult applyAndPersist(const TankSystemConfig& config);
  int findSensorIndexById(const String& id) const;

  TankConfigStore configStore_;
  TankMonitor monitor_;
  TankSystemConfig config_;
  TankServiceStatus status_;
};

}  // namespace overseer::feature::tank
