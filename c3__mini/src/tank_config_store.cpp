#include "tank/tank_config_store.hpp"

#include <ArduinoJson.h>

namespace overseer::feature::tank {

namespace {
constexpr const char* kPreferencesNamespace = "tank";
constexpr const char* kPreferencesKey = "config";
}

ConfigResult TankConfigStore::load(TankSystemConfig& outConfig) {
  String json;
  ConfigResult result = blobStore_.loadJson(kPreferencesNamespace, kPreferencesKey, json);
  if (!result.ok) {
    return result;
  }
  return deserializeConfig(json, outConfig);
}

ConfigResult TankConfigStore::save(const TankSystemConfig& config) {
  String json;
  ConfigResult result = serializeConfig(config, json);
  if (!result.ok) {
    return result;
  }
  return blobStore_.saveJson(kPreferencesNamespace, kPreferencesKey, json);
}

ConfigResult TankConfigStore::reset() {
  return blobStore_.reset(kPreferencesNamespace, kPreferencesKey);
}

ConfigResult TankConfigStore::deserializeConfig(const String& json, TankSystemConfig& outConfig) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, json);
  if (error) {
    return ConfigResult::failure(String("config parse failed: ") + error.c_str());
  }

  TankSystemConfig config = makeDefaultTankSystemConfig();
  config.feature.enabled = doc["enabled"] | config.feature.enabled;
  config.feature.version = doc["version"] | config.feature.version;
  config.feature.pollIntervalMs = doc["poll_interval_ms"] | config.feature.pollIntervalMs;
  config.feature.units = String(static_cast<const char*>(doc["units"] | config.feature.units.c_str()));
  config.feature.restoreLastStateOnBoot = doc["restore_last_state_on_boot"] | config.feature.restoreLastStateOnBoot;
  config.feature.clearHistoryOnBoot = doc["clear_history_on_boot"] | config.feature.clearHistoryOnBoot;

  config.sensors.clear();
  JsonArray sensors = doc["sensors"].as<JsonArray>();
  for (JsonObject sensorObj : sensors) {
    TankSensorConfig sensor;
    sensor.id = String(static_cast<const char*>(sensorObj["id"] | ""));
    sensor.customName = String(static_cast<const char*>(sensorObj["custom_name"] | ""));
    sensor.type = String(static_cast<const char*>(sensorObj["type"] | "custom"));
    sensor.enabled = sensorObj["enabled"] | true;
    sensor.pin = sensorObj["pin"] | 0;
    sensor.capacityGallons = sensorObj["capacity_gallons"] | 0.0f;
    sensor.usableCapacityGallons = sensorObj["usable_capacity_gallons"] | 0.0f;
    sensor.geometryType = String(static_cast<const char*>(sensorObj["geometry_type"] | "linear"));
    sensor.offsetFromBottomMm = sensorObj["offset_from_bottom_mm"] | 0;

    JsonObject calibration = sensorObj["calibration"];
    sensor.calibration.mode = String(static_cast<const char*>(calibration["mode"] | "two_point"));
    sensor.calibration.emptyValue = calibration["empty_value"] | 0;
    sensor.calibration.fullValue = calibration["full_value"] | 0;
    sensor.calibration.minValid = calibration["min_valid"] | 0;
    sensor.calibration.maxValid = calibration["max_valid"] | 50000;
    sensor.calibration.invert = calibration["invert"] | false;

    JsonObject filter = sensorObj["filter"];
    sensor.filter.mode = String(static_cast<const char*>(filter["mode"] | "ema"));
    sensor.filter.enabled = filter["enabled"] | true;
    sensor.filter.emaAlpha = filter["ema_alpha"] | 0.25f;
    sensor.filter.sampleCount = filter["sample_count"] | 8;
    sensor.filter.sampleIntervalMs = filter["sample_interval_ms"] | 25;
    sensor.filter.deadband = filter["deadband"] | 1.0f;
    sensor.filter.spikeThreshold = filter["spike_threshold"] | 10.0f;

    JsonObject thresholds = sensorObj["thresholds"];
    sensor.thresholds.emptyPercent = thresholds["empty_percent"] | 5.0f;
    sensor.thresholds.lowPercent = thresholds["low_percent"] | 15.0f;
    sensor.thresholds.mediumPercent = thresholds["medium_percent"] | 50.0f;
    sensor.thresholds.highPercent = thresholds["high_percent"] | 85.0f;
    sensor.thresholds.fullPercent = thresholds["full_percent"] | 95.0f;
    sensor.thresholds.overflowPercent = thresholds["overflow_percent"] | 99.0f;

    JsonObject faults = sensorObj["faults"];
    sensor.faults.stuckTimeoutMs = faults["stuck_timeout_ms"] | 300000UL;
    sensor.faults.noiseLimit = faults["noise_limit"] | 8.0f;
    sensor.faults.maxChangePercentPerMin = faults["max_change_percent_per_min"] | 60.0f;
    sensor.faults.faultHoldoffMs = faults["fault_holdoff_ms"] | 5000UL;

    config.sensors.push_back(sensor);
  }

  outConfig = config;
  return ConfigResult::success();
}

ConfigResult TankConfigStore::serializeConfig(const TankSystemConfig& config, String& outJson) {
  JsonDocument doc;
  doc["enabled"] = config.feature.enabled;
  doc["version"] = config.feature.version;
  doc["poll_interval_ms"] = config.feature.pollIntervalMs;
  doc["units"] = config.feature.units;
  doc["restore_last_state_on_boot"] = config.feature.restoreLastStateOnBoot;
  doc["clear_history_on_boot"] = config.feature.clearHistoryOnBoot;

  JsonArray sensors = doc["sensors"].to<JsonArray>();
  for (const TankSensorConfig& sensor : config.sensors) {
    JsonObject sensorObj = sensors.add<JsonObject>();
    sensorObj["id"] = sensor.id;
    sensorObj["custom_name"] = sensor.customName;
    sensorObj["type"] = sensor.type;
    sensorObj["enabled"] = sensor.enabled;
    sensorObj["pin"] = sensor.pin;
    sensorObj["capacity_gallons"] = sensor.capacityGallons;
    sensorObj["usable_capacity_gallons"] = sensor.usableCapacityGallons;
    sensorObj["geometry_type"] = sensor.geometryType;
    sensorObj["offset_from_bottom_mm"] = sensor.offsetFromBottomMm;

    JsonObject calibration = sensorObj["calibration"].to<JsonObject>();
    calibration["mode"] = sensor.calibration.mode;
    calibration["empty_value"] = sensor.calibration.emptyValue;
    calibration["full_value"] = sensor.calibration.fullValue;
    calibration["min_valid"] = sensor.calibration.minValid;
    calibration["max_valid"] = sensor.calibration.maxValid;
    calibration["invert"] = sensor.calibration.invert;

    JsonObject filter = sensorObj["filter"].to<JsonObject>();
    filter["mode"] = sensor.filter.mode;
    filter["enabled"] = sensor.filter.enabled;
    filter["ema_alpha"] = sensor.filter.emaAlpha;
    filter["sample_count"] = sensor.filter.sampleCount;
    filter["sample_interval_ms"] = sensor.filter.sampleIntervalMs;
    filter["deadband"] = sensor.filter.deadband;
    filter["spike_threshold"] = sensor.filter.spikeThreshold;

    JsonObject thresholds = sensorObj["thresholds"].to<JsonObject>();
    thresholds["empty_percent"] = sensor.thresholds.emptyPercent;
    thresholds["low_percent"] = sensor.thresholds.lowPercent;
    thresholds["medium_percent"] = sensor.thresholds.mediumPercent;
    thresholds["high_percent"] = sensor.thresholds.highPercent;
    thresholds["full_percent"] = sensor.thresholds.fullPercent;
    thresholds["overflow_percent"] = sensor.thresholds.overflowPercent;

    JsonObject faults = sensorObj["faults"].to<JsonObject>();
    faults["stuck_timeout_ms"] = sensor.faults.stuckTimeoutMs;
    faults["noise_limit"] = sensor.faults.noiseLimit;
    faults["max_change_percent_per_min"] = sensor.faults.maxChangePercentPerMin;
    faults["fault_holdoff_ms"] = sensor.faults.faultHoldoffMs;
  }

  outJson.reserve(kMaxTankConfigJsonBytes);
  if (serializeJson(doc, outJson) == 0) {
    return ConfigResult::failure("config serialization failed");
  }
  if (outJson.length() > kMaxTankConfigJsonBytes) {
    return ConfigResult::failure("config too large");
  }
  return ConfigResult::success();
}

}  // namespace overseer::feature::tank
