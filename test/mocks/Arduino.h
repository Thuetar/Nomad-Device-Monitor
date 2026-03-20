#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

class String {
public:
  String() = default;
  String(const char* value) : value_(value ? value : "") {}
  String(const std::string& value) : value_(value) {}
  String(int value) : value_(std::to_string(value)) {}
  String(unsigned int value) : value_(std::to_string(value)) {}
  String(long value) : value_(std::to_string(value)) {}
  String(unsigned long value) : value_(std::to_string(value)) {}
  String(float value) : value_(std::to_string(value)) {}
  String(double value) : value_(std::to_string(value)) {}

  String& operator=(const char* value) {
    value_ = value ? value : "";
    return *this;
  }

  bool operator==(const String& other) const { return value_ == other.value_; }
  bool operator!=(const String& other) const { return value_ != other.value_; }
  bool operator==(const char* other) const { return value_ == (other ? other : ""); }
  bool operator!=(const char* other) const { return !(*this == other); }

  String operator+(const String& other) const { return String(value_ + other.value_); }
  String& operator+=(const String& other) {
    value_ += other.value_;
    return *this;
  }

  const char* c_str() const { return value_.c_str(); }
  size_t length() const { return value_.size(); }
  bool isEmpty() const { return value_.empty(); }
  void reserve(size_t) {}
  void clear() { value_.clear(); }
  int read() const {
    if (read_pos_ >= value_.size()) {
      return -1;
    }
    return static_cast<unsigned char>(value_[read_pos_++]);
  }
  size_t write(uint8_t c) {
    value_.push_back(static_cast<char>(c));
    return 1;
  }
  size_t write(const uint8_t* data, size_t len) {
    value_.append(reinterpret_cast<const char*>(data), len);
    return len;
  }

  void toLowerCase() {
    std::transform(value_.begin(), value_.end(), value_.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  }

  void setCharAt(size_t index, char c) {
    if (index < value_.size()) {
      value_[index] = c;
    }
  }

  char operator[](size_t index) const { return value_[index]; }
  operator std::string() const { return value_; }

private:
  std::string value_;
  mutable size_t read_pos_ = 0;
};

inline String operator+(const char* lhs, const String& rhs) {
  return String(lhs) + rhs;
}
