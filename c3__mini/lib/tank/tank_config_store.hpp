#pragma once

#include "config/config_blob_store.hpp"
#include "config/config_store.hpp"
#include "tank/tank_config.hpp"

namespace overseer::feature::tank {

class TankConfigStore : public IConfigStore<TankSystemConfig> {
public:
  ConfigResult load(TankSystemConfig& outConfig) override;
  ConfigResult save(const TankSystemConfig& config) override;
  ConfigResult reset() override;

private:
  ConfigBlobStore blobStore_;
  ConfigResult deserializeConfig(const String& json, TankSystemConfig& outConfig);
  ConfigResult serializeConfig(const TankSystemConfig& config, String& outJson);
};

}  // namespace overseer::feature::tank
