#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <stdarg.h>

class SystemLogger;

class Logger {
 public:
  explicit Logger(const char* tag = "APP") : tag_(tag) {}

  void trace(const char* fmt, ...);
  void debug(const char* fmt, ...);
  void info(const char* fmt, ...);
  void warn(const char* fmt, ...);
  void error(const char* fmt, ...);
  void fatal(const char* fmt, ...);

 private:
  void log_(int level, const char* fmt, va_list args);

  const char* tag_;
};

class SystemLogger {
 public:
  enum Level {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
  };

  struct Config {
    bool serialEnabled = true;
    bool fileEnabled = false;
    Stream* serial = &Serial;
    const char* filePath = "/log.txt";
    size_t maxFileBytes = 0;
    bool timestamps = true;
    bool flushEachWrite = true;
  };

  static SystemLogger& instance();

  void begin(const Config& cfg);
  void setLevel(Level level);
  Level level() const;
  void setTag(const char* tag);
  const char* tag() const;

  void log(Level level, const char* tag, const char* fmt, ...);

 private:
  friend class Logger;

  SystemLogger() = default;
  void log_(Level level, const char* tag, const char* fmt, va_list args);
  void rotateIfNeeded_(size_t incomingLen);
  void writeLine_(const char* line);
  const char* levelToString_(Level level) const;

  Config cfg_;
  fs::FS* fs_ = nullptr;
  Level level_ = INFO;
  const char* defaultTag_ = "APP";
};
