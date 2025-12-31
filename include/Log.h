#pragma once

#include <functional>
#include <mutex>
#include <sstream>
#include <string>

namespace Orpheus {

/// Log severity levels
enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3, None = 4 };

/// Convert log level to string
inline const char *LogLevelToString(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warn:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  case LogLevel::None:
    return "NONE";
  }
  return "UNKNOWN";
}

/// Callback type for log messages
/// Parameters: level, message
using LogCallback = std::function<void(LogLevel, const std::string &)>;

/// Global logger with configurable callback and minimum level
class Logger {
public:
  /// Get the singleton instance
  static Logger &Instance() {
    static Logger instance;
    return instance;
  }

  /// Set the minimum log level (messages below this are ignored)
  void SetMinLevel(LogLevel level) { m_MinLevel = level; }

  /// Get the current minimum log level
  LogLevel GetMinLevel() const { return m_MinLevel; }

  /// Set a custom log callback
  /// If not set, logs go to stderr
  void SetCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Callback = std::move(callback);
  }

  /// Clear the custom callback (revert to stderr)
  void ClearCallback() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Callback = nullptr;
  }

  /// Log a message at the specified level
  void Log(LogLevel level, const std::string &message) {
    if (level < m_MinLevel)
      return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Callback) {
      m_Callback(level, message);
    } else {
      // Default: print to stderr with format "[LEVEL] message"
      fprintf(stderr, "[%s] %s\n", LogLevelToString(level), message.c_str());
    }
  }

  // Convenience methods
  void Debug(const std::string &msg) { Log(LogLevel::Debug, msg); }
  void Info(const std::string &msg) { Log(LogLevel::Info, msg); }
  void Warn(const std::string &msg) { Log(LogLevel::Warn, msg); }
  void Error(const std::string &msg) { Log(LogLevel::Error, msg); }

private:
  Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  LogLevel m_MinLevel = LogLevel::Info;
  LogCallback m_Callback;
  std::mutex m_Mutex;
};

/// Helper macros for convenient logging
#define ORPHEUS_LOG(level, msg)                                                \
  do {                                                                         \
    std::ostringstream oss;                                                    \
    oss << msg;                                                                \
    ::Orpheus::Logger::Instance().Log(level, oss.str());                       \
  } while (0)

#define ORPHEUS_DEBUG(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Debug, msg)
#define ORPHEUS_INFO(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Info, msg)
#define ORPHEUS_WARN(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Warn, msg)
#define ORPHEUS_ERROR(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Error, msg)

/// Convenience function to get the logger
inline Logger &GetLogger() { return Logger::Instance(); }

} // namespace Orpheus
