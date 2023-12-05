

#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <ostream>
#include <string>

class Logger {
  std::mutex lock;
  std::string logDir;
  std::ofstream logStream;

  void log(std::string message, std::string type);

 public:
  Logger(std::string logDir) : logDir(logDir) { initLog(); }
  void initLog();
  void log_normal(std::string message);
  void log_error(std::string message);
  ~Logger() { logStream.close(); }
};