#pragma once

#include <cstdint>
#include <string>

class Timestamp {
 public:
  explicit Timestamp(int64_t microSecondsSinceEpoch) { microSecondsSinceEpoch_ = microSecondsSinceEpoch; }

  static Timestamp now();
  std::string toString() const;

 private:
  int64_t microSecondsSinceEpoch_;
};