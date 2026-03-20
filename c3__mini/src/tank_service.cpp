#include "tank/tank_service.hpp"

#include <cctype>

#include "targets/board_config.h"

namespace overseer::feature::tank {

namespace {
bool isAllowedIdChar(char c) {
  return std::islower(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)) || c == '_' || c == '-';
}

bool isValidType(const String& type) {
  return type == "fresh" || type == "gray" || type == "black" || type == "custom";
}
}  // namespace

TankSystemConfig makeDefaultTankSystemConfig() {
  TankSystemConfig config;
  TankSensorConfig sensor;
  sensor.id = "fresh_1";
  sensor.customName = "Fresh Tank";
  sensor.type = "fresh";
  sensor.enabled = true;
  sensor.pin = WATER_LEVEL_SENSE_PIN;
  sensor.capacityGallons = 40.0f;
  sensor.usableCapacityGallons = 40.0f;
  config.sensors.push_back(sensor);
  return config;
}

String makeDefaultSensorName(const TankSensorConfig& sensor) {
  if (!sensor.customName.isEmpty()) {
    return sensor.customName;
  }

  String label = sensor.type;
  if (label.isEmpty()) {
    label = "custom";
  }
  label.toLowerCase();
  if (label.length() > 0) {
    label.setCharAt(0, static_cast<char>(std::toupper(static_cast<unsigned char>(label[0]))));
  }
  return label + " Tank";
}

void TankService::begin() {
  TankSystemConfig loadedConfig;
  ConfigResult result = configStore_.load(loadedConfig);
  if (!result.ok) {
    config_ = makeDefaultTankSystemConfig();
    status_.configLoaded = false;
    status_.configDegraded = true;
    status_.lastConfigError = result.message;
    validateConfig(config_);
    monitor_.applyConfig(config_);
    return;
  }

  result = validateConfig(loadedConfig);
  if (!result.ok) {
    config_ = makeDefaultTankSystemConfig();
    status_.configLoaded = false;
    status_.configDegraded = true;
    status_.lastConfigError = result.message;
    validateConfig(config_);
    monitor_.applyConfig(config_);
    return;
  }

  config_ = loadedConfig;
  status_.configLoaded = true;
  status_.configDegraded = false;
  status_.lastConfigError = "";
  monitor_.applyConfig(config_);
}

void TankService::service() {
  monitor_.service();
}

void TankService::forceRead() {
  monitor_.forceRead();
}

ConfigResult TankService::replaceConfig(const TankSystemConfig& config) {
  return applyAndPersist(config);
}

ConfigResult TankService::createSensor(const TankSensorConfig& sensor) {
  TankSystemConfig next = config_;
  next.sensors.push_back(sensor);
  return applyAndPersist(next);
}

ConfigResult TankService::updateSensor(const String& id, const TankSensorConfig& sensor) {
  const int index = findSensorIndexById(id);
  if (index < 0) {
    return ConfigResult::failure("sensor not found");
  }

  TankSystemConfig next = config_;
  next.sensors[static_cast<size_t>(index)] = sensor;
  if (next.sensors[static_cast<size_t>(index)].id.isEmpty()) {
    next.sensors[static_cast<size_t>(index)].id = id;
  }
  return applyAndPersist(next);
}

ConfigResult TankService::patchSensorEnabled(const String& id, bool enabled) {
  const int index = findSensorIndexById(id);
  if (index < 0) {
    return ConfigResult::failure("sensor not found");
  }

  TankSystemConfig next = config_;
  next.sensors[static_cast<size_t>(index)].enabled = enabled;
  return applyAndPersist(next);
}

ConfigResult TankService::deleteSensor(const String& id) {
  const int index = findSensorIndexById(id);
  if (index < 0) {
    return ConfigResult::failure("sensor not found");
  }

  TankSystemConfig next = config_;
  next.sensors.erase(next.sensors.begin() + index);
  return applyAndPersist(next);
}

ConfigResult TankService::updateCalibration(const String& id, const TankCalibrationConfig& calibration) {
  const int index = findSensorIndexById(id);
  if (index < 0) {
    return ConfigResult::failure("sensor not found");
  }

  TankSystemConfig next = config_;
  next.sensors[static_cast<size_t>(index)].calibration = calibration;
  return applyAndPersist(next);
}

ConfigResult TankService::clearHistory(const String& id) {
  const int index = findSensorIndexById(id);
  if (index < 0) {
    return ConfigResult::failure("sensor not found");
  }
  monitor_.clearHistory(static_cast<size_t>(index));
  return ConfigResult::success();
}

ConfigResult TankService::clearAllHistory() {
  monitor_.clearAllHistory();
  return ConfigResult::success();
}

ConfigResult TankService::validateConfig(TankSystemConfig& config) const {
  if (config.feature.version == 0) {
    config.feature.version = 1;
  }

  if (config.feature.pollIntervalMs == 0) {
    return ConfigResult::failure("poll interval must be > 0");
  }

  if (config.sensors.size() > kMaxTankSensors) {
    return ConfigResult::failure("too many tank sensors");
  }

  std::vector<String> ids;
  std::vector<uint8_t> pins;
  for (TankSensorConfig& sensor : config.sensors) {
    if (sensor.id.isEmpty()) {
      return ConfigResult::failure("sensor id is required");
    }
    for (size_t i = 0; i < sensor.id.length(); ++i) {
      if (!isAllowedIdChar(sensor.id[i])) {
        return ConfigResult::failure("sensor id contains invalid characters");
      }
    }
    for (const String& existingId : ids) {
      if (existingId == sensor.id) {
        return ConfigResult::failure("duplicate sensor id");
      }
    }
    ids.push_back(sensor.id);

    for (uint8_t existingPin : pins) {
      if (existingPin == sensor.pin) {
        return ConfigResult::failure("duplicate sensor pin");
      }
    }
    pins.push_back(sensor.pin);

    if (!isValidType(sensor.type)) {
      return ConfigResult::failure("invalid sensor type");
    }

    if (sensor.customName.isEmpty()) {
      sensor.customName = makeDefaultSensorName(sensor);
    }

    if (sensor.capacityGallons < 0.0f || sensor.usableCapacityGallons < 0.0f) {
      return ConfigResult::failure("capacity must be >= 0");
    }

    if (sensor.usableCapacityGallons <= 0.0f) {
      sensor.usableCapacityGallons = sensor.capacityGallons;
    }

    if (sensor.calibration.emptyValue >= sensor.calibration.fullValue) {
      return ConfigResult::failure("calibration empty_value must be < full_value");
    }

    if (sensor.calibration.minValid > sensor.calibration.emptyValue) {
      return ConfigResult::failure("calibration min_valid must be <= empty_value");
    }

    if (sensor.calibration.fullValue > sensor.calibration.maxValid) {
      return ConfigResult::failure("calibration full_value must be <= max_valid");
    }

    if (sensor.filter.mode != "ema") {
      return ConfigResult::failure("unsupported filter mode");
    }

    if (sensor.filter.emaAlpha < 0.0f || sensor.filter.emaAlpha > 1.0f) {
      return ConfigResult::failure("ema_alpha must be between 0 and 1");
    }

    if (!(sensor.thresholds.emptyPercent <= sensor.thresholds.lowPercent &&
          sensor.thresholds.lowPercent <= sensor.thresholds.mediumPercent &&
          sensor.thresholds.mediumPercent <= sensor.thresholds.highPercent &&
          sensor.thresholds.highPercent <= sensor.thresholds.fullPercent &&
          sensor.thresholds.fullPercent <= sensor.thresholds.overflowPercent)) {
      return ConfigResult::failure("thresholds must be monotonic");
    }
  }

  return ConfigResult::success();
}

ConfigResult TankService::applyAndPersist(const TankSystemConfig& config) {
  TankSystemConfig candidate = config;
  ConfigResult validation = validateConfig(candidate);
  if (!validation.ok) {
    return validation;
  }

  ConfigResult saveResult = configStore_.save(candidate);
  if (!saveResult.ok) {
    status_.configDegraded = true;
    status_.lastConfigError = saveResult.message;
    return saveResult;
  }

  config_ = candidate;
  status_.configLoaded = true;
  status_.configDegraded = false;
  status_.lastConfigError = "";
  monitor_.applyConfig(config_);
  return ConfigResult::success();
}

int TankService::findSensorIndexById(const String& id) const {
  for (size_t i = 0; i < config_.sensors.size(); ++i) {
    if (config_.sensors[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace overseer::feature::tank
