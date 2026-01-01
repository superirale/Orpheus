/**
 * @file Log.h
 * @brief Logging utilities for the Orpheus Audio Engine.
 *
 * Provides a configurable logging system with multiple severity levels,
 * custom callbacks, and convenience macros.
 */
#pragma once

#include <functional>
#include <mutex>
#include <sstream>
#include <string>

namespace Orpheus {

/**
 * @brief Log severity levels.
 *
 * Messages below the configured minimum level are ignored.
 */
enum class LogLevel {
  Debug = 0, ///< Debug messages (verbose)
  Info = 1,  ///< Informational messages
  Warn = 2,  ///< Warnings (potential issues)
  Error = 3, ///< Errors (failures)
  None = 4   ///< Disable all logging
};

/**
 * @brief Converts a log level to a human-readable string.
 * @param level The log level to convert.
 * @return C-string name of the log level.
 */
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

/**
 * @brief Callback type for log messages.
 *
 * @param level The severity level of the message.
 * @param message The log message content.
 */
using LogCallback = std::function<void(LogLevel, const std::string &)>;

/**
 * @brief Global logger with configurable callback and minimum level.
 *
 * Thread-safe singleton logger that outputs to stderr by default,
 * but can be configured with a custom callback for integration with
 * game engine logging systems.
 *
 * @par Example Usage:
 * @code
 * // Set minimum level
 * Orpheus::Logger::Instance().SetMinLevel(Orpheus::LogLevel::Debug);
 *
 * // Set custom callback
 * Orpheus::Logger::Instance().SetCallback([](LogLevel lvl, const std::string&
 * msg) { myGameEngine.log(msg);
 * });
 *
 * // Use convenience macros
 * ORPHEUS_INFO("Sound loaded: " << filename);
 * ORPHEUS_ERROR("Failed to play: " << eventName);
 * @endcode
 */
class Logger {
public:
  /**
   * @brief Get the singleton instance.
   * @return Reference to the global Logger.
   */
  static Logger &Instance() {
    static Logger instance;
    return instance;
  }

  /**
   * @brief Set the minimum log level.
   *
   * Messages below this level are ignored.
   * @param level Minimum level to log.
   */
  void SetMinLevel(LogLevel level) { m_MinLevel = level; }

  /**
   * @brief Get the current minimum log level.
   * @return The minimum log level.
   */
  LogLevel GetMinLevel() const { return m_MinLevel; }

  /**
   * @brief Set a custom log callback.
   *
   * If set, log messages are sent to this callback instead of stderr.
   * @param callback Function to receive log messages.
   */
  void SetCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Callback = std::move(callback);
  }

  /**
   * @brief Clear the custom callback (revert to stderr).
   */
  void ClearCallback() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Callback = nullptr;
  }

  /**
   * @brief Log a message at the specified level.
   * @param level The severity level.
   * @param message The message to log.
   */
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

  /**
   * @brief Log a debug message.
   * @param msg The message to log.
   */
  void Debug(const std::string &msg) { Log(LogLevel::Debug, msg); }

  /**
   * @brief Log an info message.
   * @param msg The message to log.
   */
  void Info(const std::string &msg) { Log(LogLevel::Info, msg); }

  /**
   * @brief Log a warning message.
   * @param msg The message to log.
   */
  void Warn(const std::string &msg) { Log(LogLevel::Warn, msg); }

  /**
   * @brief Log an error message.
   * @param msg The message to log.
   */
  void Error(const std::string &msg) { Log(LogLevel::Error, msg); }

private:
  Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  LogLevel m_MinLevel = LogLevel::Info;
  LogCallback m_Callback;
  std::mutex m_Mutex;
};

/**
 * @def ORPHEUS_LOG(level, msg)
 * @brief Log a message at the specified level.
 * @param level The LogLevel.
 * @param msg Stream expression for the message.
 */
#define ORPHEUS_LOG(level, msg)                                                \
  do {                                                                         \
    std::ostringstream oss;                                                    \
    oss << msg;                                                                \
    ::Orpheus::Logger::Instance().Log(level, oss.str());                       \
  } while (0)

/**
 * @def ORPHEUS_DEBUG(msg)
 * @brief Log a debug message.
 * @param msg Stream expression for the message.
 */
#define ORPHEUS_DEBUG(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Debug, msg)

/**
 * @def ORPHEUS_INFO(msg)
 * @brief Log an info message.
 * @param msg Stream expression for the message.
 */
#define ORPHEUS_INFO(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Info, msg)

/**
 * @def ORPHEUS_WARN(msg)
 * @brief Log a warning message.
 * @param msg Stream expression for the message.
 */
#define ORPHEUS_WARN(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Warn, msg)

/**
 * @def ORPHEUS_ERROR(msg)
 * @brief Log an error message.
 * @param msg Stream expression for the message.
 */
#define ORPHEUS_ERROR(msg) ORPHEUS_LOG(::Orpheus::LogLevel::Error, msg)

/**
 * @brief Convenience function to get the logger.
 * @return Reference to the global Logger instance.
 */
inline Logger &GetLogger() { return Logger::Instance(); }

} // namespace Orpheus
