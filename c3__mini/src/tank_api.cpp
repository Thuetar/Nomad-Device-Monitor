#include "tank/tank_api.hpp"

#include <ESPAsyncWebServer.h>

#include "tank/tank_monitor.hpp"

namespace overseer::feature::tank {

void registerTankApi(AsyncWebServer& server, TankMonitor& monitor) {
  server.on("/api/tank", HTTP_GET, [&monitor](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->printf("{");
    response->printf("\"enabled\":%s,", monitor.config().enabled ? "true" : "false");
    response->printf("\"version\":%lu,", static_cast<unsigned long>(monitor.config().version));
    response->printf("\"poll_interval_ms\":%lu,", static_cast<unsigned long>(monitor.config().pollIntervalMs));
    response->printf("\"units\":\"%s\",", monitor.config().units.c_str());
    response->printf("\"last_poll_ms\":%lu,", static_cast<unsigned long>(monitor.lastPollMs()));
    response->printf("\"sensor_count\":%u,", static_cast<unsigned>(monitor.sensorCount()));
    response->printf("\"healthy_sensor_count\":%u,", static_cast<unsigned>(monitor.healthySensorCount()));
    response->printf("\"sensors\":[");

    const auto& sensors = monitor.config().sensors;
    const auto& states = monitor.states();
    for (size_t i = 0; i < sensors.size() && i < states.size(); ++i) {
      if (i > 0) {
        response->printf(",");
      }

      const TankSensorConfig& cfg = sensors[i];
      const TankSensorState& state = states[i];
      response->printf("{");
      response->printf("\"id\":\"%s\",", cfg.id.c_str());
      response->printf("\"type\":\"%s\",", toString(cfg.type));
      response->printf("\"custom_name\":\"%s\",", cfg.customName.c_str());
      response->printf("\"enabled\":%s,", cfg.enabled ? "true" : "false");
      response->printf("\"pin\":%u,", cfg.pin);
      response->printf("\"status\":\"%s\",", toString(state.status));
      response->printf("\"reading\":{");
      response->printf("\"last_read_ms\":%lu,", static_cast<unsigned long>(state.reading.lastReadMs));
      response->printf("\"raw\":%ld,", static_cast<long>(state.reading.raw));
      response->printf("\"filtered\":%.4f,", static_cast<double>(state.reading.filtered));
      response->printf("\"percent\":%.4f,", static_cast<double>(state.reading.percent));
      response->printf("\"volume_gallons\":%.4f,", static_cast<double>(state.reading.volumeGallons));
      response->printf("\"valid\":%s", state.reading.valid ? "true" : "false");
      response->printf("},");
      response->printf("\"history\":{");
      response->printf("\"lowest_percent\":%.4f,", static_cast<double>(state.history.lowestPercent));
      response->printf("\"highest_percent\":%.4f,", static_cast<double>(state.history.highestPercent));
      response->printf("\"samples_taken\":%lu,", static_cast<unsigned long>(state.history.samplesTaken));
      response->printf("\"read_error_count\":%lu", static_cast<unsigned long>(state.history.readErrorCount));
      response->printf("}");
      response->printf("}");
    }

    response->printf("]}");
    request->send(response);
  });

  server.on("/api/tank/config", HTTP_GET, [&monitor](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->printf("{");
    response->printf("\"enabled\":%s,", monitor.config().enabled ? "true" : "false");
    response->printf("\"version\":%lu,", static_cast<unsigned long>(monitor.config().version));
    response->printf("\"poll_interval_ms\":%lu,", static_cast<unsigned long>(monitor.config().pollIntervalMs));
    response->printf("\"units\":\"%s\",", monitor.config().units.c_str());
    response->printf("\"sensors\":[");

    const auto& sensors = monitor.config().sensors;
    for (size_t i = 0; i < sensors.size(); ++i) {
      if (i > 0) {
        response->printf(",");
      }

      const TankSensorConfig& cfg = sensors[i];
      response->printf("{");
      response->printf("\"id\":\"%s\",", cfg.id.c_str());
      response->printf("\"type\":\"%s\",", toString(cfg.type));
      response->printf("\"custom_name\":\"%s\",", cfg.customName.c_str());
      response->printf("\"enabled\":%s,", cfg.enabled ? "true" : "false");
      response->printf("\"pin\":%u,", cfg.pin);
      response->printf("\"capacity_gallons\":%.4f,", static_cast<double>(cfg.capacityGallons));
      response->printf("\"calibration\":{");
      response->printf("\"empty_value\":%ld,", static_cast<long>(cfg.calibration.emptyValue));
      response->printf("\"full_value\":%ld,", static_cast<long>(cfg.calibration.fullValue));
      response->printf("\"min_valid\":%ld,", static_cast<long>(cfg.calibration.minValid));
      response->printf("\"max_valid\":%ld,", static_cast<long>(cfg.calibration.maxValid));
      response->printf("\"invert\":%s", cfg.calibration.invert ? "true" : "false");
      response->printf("},");
      response->printf("\"filter\":{");
      response->printf("\"enabled\":%s,", cfg.filter.enabled ? "true" : "false");
      response->printf("\"ema_alpha\":%.4f", static_cast<double>(cfg.filter.emaAlpha));
      response->printf("}");
      response->printf("}");
    }

    response->printf("]}");
    request->send(response);
  });
}

}  // namespace overseer::feature::tank
