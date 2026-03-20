#include <string>

#include "boost/ut.hpp"
#include "Preferences.h"

#include "../../c3__mini/src/config_blob_store.cpp"
#include "../../c3__mini/src/tank_config_store.cpp"
#include "../../c3__mini/src/tank_service.cpp"

int main() {
  using namespace boost::ut;
  using namespace overseer::feature::tank;

  auto makeValidSensor = [] {
    TankSensorConfig sensor;
    sensor.id = "fresh_1";
    sensor.customName = "Fresh Tank";
    sensor.type = "fresh";
    sensor.enabled = true;
    sensor.pin = 4;
    sensor.capacityGallons = 40.0f;
    sensor.usableCapacityGallons = 38.0f;
    sensor.calibration.emptyValue = 1800;
    sensor.calibration.fullValue = 3200;
    sensor.calibration.minValid = 1500;
    sensor.calibration.maxValid = 3400;
    sensor.filter.emaAlpha = 0.25f;
    return sensor;
  };

  "tank config store round-trips json blob"_test = [&] {
    preferences_mock::clear();

    TankSystemConfig config;
    config.feature.enabled = true;
    config.feature.version = 2;
    config.feature.pollIntervalMs = 1500;
    config.feature.units = "percent";
    config.sensors = {makeValidSensor()};

    TankConfigStore store;
    const ConfigResult saveResult = store.save(config);
    expect(saveResult.ok) << static_cast<std::string>(saveResult.message);

    TankSystemConfig loaded;
    const ConfigResult loadResult = store.load(loaded);
    expect(loadResult.ok) << static_cast<std::string>(loadResult.message);
    expect(loaded.feature.version == 2_u);
    expect(loaded.feature.pollIntervalMs == 1500_u);
    expect(loaded.sensors.size() == 1_u);
    expect(loaded.sensors[0].id == "fresh_1");
    expect(loaded.sensors[0].pin == static_cast<uint8_t>(4));
    expect(loaded.sensors[0].calibration.fullValue == 3200_i);
  };

  "tank service rejects duplicate pins"_test = [&] {
    preferences_mock::clear();

    TankService service;
    service.begin();

    TankSystemConfig config;
    TankSensorConfig first = makeValidSensor();
    TankSensorConfig second = makeValidSensor();
    second.id = "gray_1";
    second.type = "gray";
    second.pin = first.pin;
    second.customName = "Gray Tank";
    config.sensors = {first, second};

    const ConfigResult result = service.replaceConfig(config);
    expect(!result.ok);
    expect(result.message == "duplicate sensor pin");
  };

  "tank service rejects invalid sensor id characters"_test = [&] {
    preferences_mock::clear();

    TankService service;
    service.begin();

    TankSystemConfig config;
    TankSensorConfig sensor = makeValidSensor();
    sensor.id = "Fresh 1";
    config.sensors = {sensor};

    const ConfigResult result = service.replaceConfig(config);
    expect(!result.ok);
    expect(result.message == "sensor id contains invalid characters");
  };

  "tank service create update delete persists config"_test = [&] {
    preferences_mock::clear();

    TankService service;
    service.begin();

    TankSystemConfig base;
    base.sensors = {makeValidSensor()};
    expect(service.replaceConfig(base).ok);

    TankSensorConfig extra = makeValidSensor();
    extra.id = "gray_1";
    extra.type = "gray";
    extra.pin = 5;
    extra.customName = "Gray Tank";
    expect(service.createSensor(extra).ok);
    expect(service.config().sensors.size() == 2_u);

    expect(service.patchSensorEnabled("gray_1", false).ok);
    expect(!service.config().sensors[1].enabled);

    expect(service.deleteSensor("gray_1").ok);
    expect(service.config().sensors.size() == 1_u);

    TankConfigStore store;
    TankSystemConfig loaded;
    expect(store.load(loaded).ok);
    expect(loaded.sensors.size() == 1_u);
    expect(loaded.sensors[0].id == "fresh_1");
  };
}
