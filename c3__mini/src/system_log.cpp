#include "system_log.h"

static const size_t kLineBufferSize = 256;

SystemLogger& SystemLogger::instance() {
  static SystemLogger inst;
  return inst;
}

void SystemLogger::begin(const Config& cfg) {
  cfg_ = cfg;
  fs_ = &SPIFFS;
  if (cfg_.fileEnabled) {
    SPIFFS.begin(true);
  }
}

void SystemLogger::setLevel(Level level) {
  level_ = level;
}

SystemLogger::Level SystemLogger::level() const {
  return level_;
}

void SystemLogger::setTag(const char* tag) {
  defaultTag_ = (tag && tag[0] != '\0') ? tag : "APP";
}

const char* SystemLogger::tag() const {
  return defaultTag_;
}

const char* SystemLogger::levelToString_(Level level) const {
  switch (level) {
    case TRACE:
      return "TRACE";
    case DEBUG:
      return "DEBUG";
    case INFO:
      return "INFO";
    case WARN:
      return "WARN";
    case ERROR:
      return "ERROR";
    case FATAL:
      return "FATAL";
    default:
      return "INFO";
  }
}

void SystemLogger::log(Level level, const char* tag, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(level, tag, fmt, args);
  va_end(args);
}

void SystemLogger::log_(Level level, const char* tag, const char* fmt, va_list args) {
  if (level < level_) {
    return;
  }

  char msg[kLineBufferSize];
  vsnprintf(msg, sizeof(msg), fmt, args);

  char line[kLineBufferSize];
  if (cfg_.timestamps) {
    snprintf(line, sizeof(line), "[%lu][%s][%s] %s",
             static_cast<unsigned long>(millis()),
             levelToString_(level),
             (tag && tag[0] != '\0') ? tag : defaultTag_,
             msg);
  } else {
    snprintf(line, sizeof(line), "[%s][%s] %s",
             levelToString_(level),
             (tag && tag[0] != '\0') ? tag : defaultTag_,
             msg);
  }

  writeLine_(line);
}

void SystemLogger::writeLine_(const char* line) {
  if (cfg_.serialEnabled && cfg_.serial) {
    cfg_.serial->println(line);
  }

  if (!cfg_.fileEnabled || fs_ == nullptr) {
    return;
  }

  rotateIfNeeded_(strlen(line) + 1);

  File f = fs_->open(cfg_.filePath, FILE_APPEND);
  if (!f) {
    return;
  }
  f.println(line);
  if (cfg_.flushEachWrite) {
    f.flush();
  }
  f.close();
}

void SystemLogger::rotateIfNeeded_(size_t incomingLen) {
  if (cfg_.maxFileBytes == 0 || fs_ == nullptr) {
    return;
  }

  File f = fs_->open(cfg_.filePath, FILE_READ);
  size_t size = f ? f.size() : 0;
  if (f) {
    f.close();
  }

  if (size + incomingLen <= cfg_.maxFileBytes) {
    return;
  }

  String backup = String(cfg_.filePath) + ".1";
  if (fs_->exists(backup)) {
    fs_->remove(backup);
  }
  if (fs_->exists(cfg_.filePath)) {
    fs_->rename(cfg_.filePath, backup);
  }
}

void Logger::log_(int level, const char* fmt, va_list args) {
  SystemLogger::instance().log_(static_cast<SystemLogger::Level>(level), tag_, fmt, args);
}

void Logger::trace(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(SystemLogger::TRACE, fmt, args);
  va_end(args);
}

void Logger::debug(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(SystemLogger::DEBUG, fmt, args);
  va_end(args);
}

void Logger::info(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(SystemLogger::INFO, fmt, args);
  va_end(args);
}

void Logger::warn(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(SystemLogger::WARN, fmt, args);
  va_end(args);
}

void Logger::error(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(SystemLogger::ERROR, fmt, args);
  va_end(args);
}

void Logger::fatal(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_(SystemLogger::FATAL, fmt, args);
  va_end(args);
}
