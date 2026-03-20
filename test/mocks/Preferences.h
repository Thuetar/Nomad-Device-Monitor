#pragma once

#include <map>
#include <string>

#include "Arduino.h"

namespace preferences_mock {
inline std::map<std::string, std::string> store;

inline void clear() {
  store.clear();
}
}  // namespace preferences_mock

class Preferences {
public:
  bool begin(const char* preferencesNamespace, bool readOnly) {
    ns_ = preferencesNamespace ? preferencesNamespace : "";
    readOnly_ = readOnly;
    return true;
  }

  bool isKey(const char* key) const {
    return preferences_mock::store.count(composeKey(key)) > 0;
  }

  String getString(const char* key, const char* defaultValue = "") const {
    const auto it = preferences_mock::store.find(composeKey(key));
    if (it == preferences_mock::store.end()) {
      return String(defaultValue);
    }
    return String(it->second);
  }

  size_t putString(const char* key, const String& value) {
    if (readOnly_) {
      return 0;
    }
    const std::string encoded = static_cast<std::string>(value);
    preferences_mock::store[composeKey(key)] = encoded;
    return encoded.size();
  }

  void remove(const char* key) {
    if (!readOnly_) {
      preferences_mock::store.erase(composeKey(key));
    }
  }

  void end() {}

private:
  std::string composeKey(const char* key) const {
    return ns_ + ":" + (key ? key : "");
  }

  std::string ns_;
  bool readOnly_ = false;
};
