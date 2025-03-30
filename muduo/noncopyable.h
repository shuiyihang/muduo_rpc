#pragma once

class noncopyable {
 public:
  noncopyable(const noncopyable &other) = delete;
  noncopyable &operator=(const noncopyable &other) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};