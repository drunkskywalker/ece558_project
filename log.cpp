#include "log.hpp"

void Logger::initLog() {
  try {
    logStream.open(logDir.c_str(), std::ios::out | std::ios::app);
    if (!logStream.is_open()) {
      throw std::runtime_error("Failed to open log file");
    }
  }
  catch (std::exception & e) {
    std::cerr << "Exception in initLog(): " << e.what() << "\n";
  }
}

void Logger::log(std::string message, std::string type) {
  try {
    std::lock_guard<std::mutex> guard(lock);

    std::string timestamp = std::to_string(time(NULL));
    std::string lgMsg = "[" + timestamp + "][" + type + "]: " + message + "\n";
    logStream << lgMsg;
  }
  catch (std::exception & e) {
    std::cerr << "Exception in log(): " << e.what() << "\n";
  }
}

void Logger::log_normal(std::string message) {
  log(message, "MESSAGE");
}

void Logger::log_error(std::string message) {
  log(message, "ERROR");
}
