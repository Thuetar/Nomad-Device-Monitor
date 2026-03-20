#pragma once

#include <Arduino.h>

struct ConfigResult {
  bool ok = false;
  String message;

  static ConfigResult success() {
    return {true, ""};
  }

  static ConfigResult failure(const String& reason) {
    return {false, reason};
  }
};
