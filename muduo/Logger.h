#pragma once

#include <string>
#include "noncopyable.h"

class Logger : noncopyable {
 public:
  enum class LogLevel { INFO, ERROR, FATAL, DEBUG };
  static Logger &get_instance();

  void setLogLevel(LogLevel level) { m_log_level = level; }
  void log(std::string msg);

 private:
  LogLevel m_log_level;
};

#define LOG_INFO(format, ...)                          \
  do {                                                 \
    Logger &logger = Logger::get_instance();           \
    logger.setLogLevel(Logger::LogLevel::INFO);        \
    char msg[1024] = {0};                              \
    snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
    logger.log(msg);                                   \
  } while (0)

#define LOG_ERROR(format, ...)                         \
  do {                                                 \
    Logger &logger = Logger::get_instance();           \
    logger.setLogLevel(Logger::LogLevel::ERROR);       \
    char msg[1024] = {0};                              \
    snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
    logger.log(msg);                                   \
  } while (0)

#define LOG_FATAL(format, ...)                         \
  do {                                                 \
    Logger &logger = Logger::get_instance();           \
    logger.setLogLevel(Logger::LogLevel::FATAL);       \
    char msg[1024] = {0};                              \
    snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
    logger.log(msg);                                   \
    exit(-1);                                          \
  } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(format, ...)                         \
  do {                                                 \
    Logger &logger = Logger::get_instance();           \
    logger.setLogLevel(Logger::LogLevel::DEBUG);       \
    char msg[1024] = {0};                              \
    snprintf(msg, sizeof(msg), format, ##__VA_ARGS__); \
    logger.log(msg);                                   \
  } while (0)
#else
#define LOG_DEBUG(format, ...)
#endif
