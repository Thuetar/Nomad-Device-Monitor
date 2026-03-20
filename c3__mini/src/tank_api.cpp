#include "tank/tank_api.hpp"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include "tank/tank_service.hpp"

namespace overseer::feature::tank {

namespace {

String statusToString(SensorStatus value) {
  return String(toString(value));
}

void appendSensorConfig(JsonArray sensors, const TankSensorConfig& sensor) {
  JsonObject sensorObj = sensors.add<JsonObject>();
  sensorObj["id"] = sensor.id;
  sensorObj["type"] = sensor.type;
  sensorObj["custom_name"] = sensor.customName;
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

void writeConfigDocument(JsonDocument& doc, const TankService& service) {
  doc["enabled"] = service.config().feature.enabled;
  doc["version"] = service.config().feature.version;
  doc["poll_interval_ms"] = service.config().feature.pollIntervalMs;
  doc["units"] = service.config().feature.units;
  doc["restore_last_state_on_boot"] = service.config().feature.restoreLastStateOnBoot;
  doc["clear_history_on_boot"] = service.config().feature.clearHistoryOnBoot;

  JsonArray sensors = doc["sensors"].to<JsonArray>();
  for (const TankSensorConfig& sensor : service.config().sensors) {
    appendSensorConfig(sensors, sensor);
  }
}

TankSensorConfig sensorFromJson(JsonObject sensorObj) {
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

  return sensor;
}

bool parseRequestJson(uint8_t* data, size_t len, JsonDocument& doc, String& errorText) {
  const DeserializationError error = deserializeJson(doc, data, len);
  if (error) {
    errorText = error.c_str();
    return false;
  }
  return true;
}

void sendJson(AsyncWebServerRequest* request, const JsonDocument& doc, int statusCode = 200) {
  String body;
  serializeJson(doc, body);
  request->send(statusCode, "application/json", body);
}

void sendResult(AsyncWebServerRequest* request, const ConfigResult& result, int successCode = 200) {
  JsonDocument doc;
  doc["ok"] = result.ok;
  if (!result.message.isEmpty()) {
    doc["message"] = result.message;
  }
  sendJson(request, doc, result.ok ? successCode : 400);
}

}  // namespace

void registerTankApi(AsyncWebServer& server, TankService& service) {
  server.on("/api/tank", HTTP_GET, [&service](AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["enabled"] = service.config().feature.enabled;
    doc["version"] = service.config().feature.version;
    doc["poll_interval_ms"] = service.config().feature.pollIntervalMs;
    doc["units"] = service.config().feature.units;
    doc["last_poll_ms"] = service.lastPollMs();
    doc["sensor_count"] = service.sensorCount();
    doc["healthy_sensor_count"] = service.healthySensorCount();
    doc["config_loaded"] = service.status().configLoaded;
    doc["config_degraded"] = service.status().configDegraded;
    if (!service.status().lastConfigError.isEmpty()) {
      doc["last_config_error"] = service.status().lastConfigError;
    }

    JsonArray sensors = doc["sensors"].to<JsonArray>();
    const auto& cfgs = service.config().sensors;
    const auto& states = service.states();
    for (size_t i = 0; i < cfgs.size() && i < states.size(); ++i) {
      JsonObject sensorObj = sensors.add<JsonObject>();
      sensorObj["id"] = cfgs[i].id;
      sensorObj["type"] = cfgs[i].type;
      sensorObj["custom_name"] = cfgs[i].customName;
      sensorObj["enabled"] = cfgs[i].enabled;
      sensorObj["pin"] = cfgs[i].pin;
      sensorObj["status"] = statusToString(states[i].status);

      JsonObject reading = sensorObj["reading"].to<JsonObject>();
      reading["last_read_ms"] = states[i].reading.lastReadMs;
      reading["raw"] = states[i].reading.raw;
      reading["filtered"] = states[i].reading.filtered;
      reading["percent"] = states[i].reading.percent;
      reading["volume_gallons"] = states[i].reading.volumeGallons;
      reading["valid"] = states[i].reading.valid;

      JsonObject history = sensorObj["history"].to<JsonObject>();
      history["lowest_percent"] = states[i].history.lowestPercent;
      history["highest_percent"] = states[i].history.highestPercent;
      history["samples_taken"] = states[i].history.samplesTaken;
      history["read_error_count"] = states[i].history.readErrorCount;
    }

    sendJson(request, doc);
  });

  server.on("/api/tank/config", HTTP_GET, [&service](AsyncWebServerRequest* request) {
    JsonDocument doc;
    writeConfigDocument(doc, service);
    sendJson(request, doc);
  });

  server.on("/api/tank/config", HTTP_POST,
    [&service](AsyncWebServerRequest* request) { (void)service; },
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"chunked body not supported\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      TankSystemConfig config;
      config.feature.enabled = doc["enabled"] | true;
      config.feature.version = doc["version"] | 1;
      config.feature.pollIntervalMs = doc["poll_interval_ms"] | 1000UL;
      config.feature.units = String(static_cast<const char*>(doc["units"] | "percent"));
      config.feature.restoreLastStateOnBoot = doc["restore_last_state_on_boot"] | false;
      config.feature.clearHistoryOnBoot = doc["clear_history_on_boot"] | true;

      JsonArray sensors = doc["sensors"].as<JsonArray>();
      for (JsonObject sensorObj : sensors) {
        config.sensors.push_back(sensorFromJson(sensorObj));
      }

      sendResult(request, service.replaceConfig(config));
    });

  server.on("/api/tank/sensors", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"chunked body not supported\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      sendResult(request, service.createSensor(sensorFromJson(doc.as<JsonObject>())), 201);
    });

  server.on("/api/tank/sensor/update", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"invalid request\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      const String sensorId = String(static_cast<const char*>(doc["id"] | ""));
      sendResult(request, service.updateSensor(sensorId, sensorFromJson(doc.as<JsonObject>())));
    });

  server.on("/api/tank/sensor/enabled", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"invalid request\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      const String sensorId = String(static_cast<const char*>(doc["id"] | ""));
      sendResult(request, service.patchSensorEnabled(sensorId, doc["enabled"] | false));
    });

  server.on("/api/tank/sensor/delete", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"invalid request\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      const String sensorId = String(static_cast<const char*>(doc["id"] | ""));
      sendResult(request, service.deleteSensor(sensorId));
    });

  server.on("/api/tank/sensor/calibration", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"invalid request\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      TankCalibrationConfig calibration;
      calibration.mode = String(static_cast<const char*>(doc["mode"] | "two_point"));
      calibration.emptyValue = doc["empty_value"] | 0;
      calibration.fullValue = doc["full_value"] | 0;
      calibration.minValid = doc["min_valid"] | 0;
      calibration.maxValid = doc["max_valid"] | 50000;
      calibration.invert = doc["invert"] | false;
      const String sensorId = String(static_cast<const char*>(doc["id"] | ""));
      sendResult(request, service.updateCalibration(sensorId, calibration));
    });

  server.on("/api/tank/sensor/clear-history", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [&service](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index != 0 || len != total) {
        request->send(400, "application/json", "{\"ok\":false,\"message\":\"invalid request\"}");
        return;
      }

      JsonDocument doc;
      String errorText;
      if (!parseRequestJson(data, len, doc, errorText)) {
        request->send(400, "application/json", String("{\"ok\":false,\"message\":\"") + errorText + "\"}");
        return;
      }

      const String sensorId = String(static_cast<const char*>(doc["id"] | ""));
      sendResult(request, service.clearHistory(sensorId));
    });

  server.on("/api/tank/clear-history", HTTP_POST, [&service](AsyncWebServerRequest* request) {
    sendResult(request, service.clearAllHistory());
  });

  server.on("/api/tank/force-read", HTTP_POST, [&service](AsyncWebServerRequest* request) {
    service.forceRead();
    sendResult(request, ConfigResult::success());
  });
}

}  // namespace overseer::feature::tank
