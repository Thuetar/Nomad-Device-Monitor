#pragma once

#include "config_result.hpp"

template <typename TConfig>
class IConfigStore {
public:
  virtual ~IConfigStore() = default;
  virtual ConfigResult load(TConfig& outConfig) = 0;
  virtual ConfigResult save(const TConfig& config) = 0;
  virtual ConfigResult reset() = 0;
};
