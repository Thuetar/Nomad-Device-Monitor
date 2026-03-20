#include "config/config_blob_store.hpp"

#include <Preferences.h>

ConfigResult ConfigBlobStore::loadJson(const char* preferencesNamespace, const char* key, String& outJson) {
  Preferences preferences;
  if (!preferences.begin(preferencesNamespace, true)) {
    return ConfigResult::failure("preferences open failed");
  }

  if (!preferences.isKey(key)) {
    preferences.end();
    outJson = "";
    return ConfigResult::failure("config missing");
  }

  outJson = preferences.getString(key, "");
  preferences.end();
  return ConfigResult::success();
}

ConfigResult ConfigBlobStore::saveJson(const char* preferencesNamespace, const char* key, const String& json) {
  Preferences preferences;
  if (!preferences.begin(preferencesNamespace, false)) {
    return ConfigResult::failure("preferences open failed");
  }

  const bool saved = preferences.putString(key, json) > 0;
  preferences.end();
  if (!saved) {
    return ConfigResult::failure("config save failed");
  }
  return ConfigResult::success();
}

ConfigResult ConfigBlobStore::reset(const char* preferencesNamespace, const char* key) {
  Preferences preferences;
  if (!preferences.begin(preferencesNamespace, false)) {
    return ConfigResult::failure("preferences open failed");
  }

  preferences.remove(key);
  preferences.end();
  return ConfigResult::success();
}
