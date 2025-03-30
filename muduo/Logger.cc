#include "Logger.h"
#include <iostream>
#include "Timestamp.h"

Logger &Logger::get_instance() {
  static Logger logger;
  return logger;
}

void Logger::log(std::string msg) {
  switch (m_log_level) {
    case LogLevel::INFO:
      std::cout << "[INFO]";
      break;
    case LogLevel::ERROR:
      std::cout << "[ERROR]";
      break;
    case LogLevel::FATAL:
      std::cout << "[FATAL]";
      break;
    case LogLevel::DEBUG:
      std::cout << "[DEBUG]";
      break;
  }

  std::cout << Timestamp::now().toString() << ": " << msg << std::endl;
}
