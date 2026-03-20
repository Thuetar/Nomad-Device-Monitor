#pragma once

#include <Arduino.h>

#include "config_result.hpp"

class ConfigBlobStore {
public:
  ConfigResult loadJson(const char* preferencesNamespace, const char* key, String& outJson);
  ConfigResult saveJson(const char* preferencesNamespace, const char* key, const String& json);
  ConfigResult reset(const char* preferencesNamespace, const char* key);
};
