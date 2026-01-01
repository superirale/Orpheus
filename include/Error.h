/**
 * @file Error.h
 * @brief Error handling utilities for the Orpheus Audio Engine.
 *
 * Provides error codes, error objects, and a Result type for operations
 * that can fail. This enables consistent error handling throughout the engine.
 */
#pragma once

#include <string>
#include <variant>
#include <optional>

namespace Orpheus {

/**
 * @brief Error codes for Orpheus operations.
 *
 * Categorized error codes covering initialization, resources,
 * playback, bus/zone management, parameters, and general errors.
 */
enum class ErrorCode {
  Ok = 0, ///< Operation succeeded

  // Initialization errors
  EngineInitFailed,   ///< Audio engine failed to initialize
  AlreadyInitialized, ///< Engine already initialized
  NotInitialized,     ///< Engine not yet initialized

  // Resource errors
  FileNotFound,   ///< Audio file not found
  InvalidPath,    ///< Invalid file path
  JsonParseError, ///< JSON parsing failed
  InvalidFormat,  ///< Invalid audio format

  // Playback errors
  EventNotFound,         ///< Audio event not registered
  VoiceAllocationFailed, ///< Could not allocate voice
  InvalidHandle,         ///< Invalid audio handle
  PlaybackFailed,        ///< Audio playback failed

  // Bus/Zone errors
  BusNotFound,         ///< Bus not found
  BusAlreadyExists,    ///< Bus already exists
  ReverbBusNotFound,   ///< Reverb bus not found
  ReverbBusInitFailed, ///< Reverb bus initialization failed
  SnapshotNotFound,    ///< Snapshot not found
  ZoneNotFound,        ///< Zone not found
  ListenerNotFound,    ///< Listener not found

  // Parameter errors
  InvalidParameter, ///< Invalid parameter name
  OutOfRange,       ///< Value out of range

  // General
  Unknown ///< Unknown error
};

/**
 * @brief Converts an error code to a human-readable string.
 * @param code The error code to convert.
 * @return C-string representation of the error code.
 */
inline const char *ErrorCodeToString(ErrorCode code) {
  switch (code) {
  case ErrorCode::Ok:
    return "Ok";
  case ErrorCode::EngineInitFailed:
    return "EngineInitFailed";
  case ErrorCode::AlreadyInitialized:
    return "AlreadyInitialized";
  case ErrorCode::NotInitialized:
    return "NotInitialized";
  case ErrorCode::FileNotFound:
    return "FileNotFound";
  case ErrorCode::InvalidPath:
    return "InvalidPath";
  case ErrorCode::JsonParseError:
    return "JsonParseError";
  case ErrorCode::InvalidFormat:
    return "InvalidFormat";
  case ErrorCode::EventNotFound:
    return "EventNotFound";
  case ErrorCode::VoiceAllocationFailed:
    return "VoiceAllocationFailed";
  case ErrorCode::InvalidHandle:
    return "InvalidHandle";
  case ErrorCode::PlaybackFailed:
    return "PlaybackFailed";
  case ErrorCode::BusNotFound:
    return "BusNotFound";
  case ErrorCode::BusAlreadyExists:
    return "BusAlreadyExists";
  case ErrorCode::ReverbBusNotFound:
    return "ReverbBusNotFound";
  case ErrorCode::ReverbBusInitFailed:
    return "ReverbBusInitFailed";
  case ErrorCode::SnapshotNotFound:
    return "SnapshotNotFound";
  case ErrorCode::ZoneNotFound:
    return "ZoneNotFound";
  case ErrorCode::ListenerNotFound:
    return "ListenerNotFound";
  case ErrorCode::InvalidParameter:
    return "InvalidParameter";
  case ErrorCode::OutOfRange:
    return "OutOfRange";
  case ErrorCode::Unknown:
    return "Unknown";
  }
  return "Unknown";
}

/**
 * @brief Error class containing an error code and optional message.
 *
 * Encapsulates error information with a code for programmatic handling
 * and an optional message for additional context.
 */
class Error {
public:
  /**
   * @brief Default constructor creating an Unknown error.
   */
  Error() : m_Code(ErrorCode::Unknown) {}

  /**
   * @brief Construct an error with just a code.
   * @param code The error code.
   */
  Error(ErrorCode code) : m_Code(code) {}

  /**
   * @brief Construct an error with a code and message.
   * @param code The error code.
   * @param message Descriptive error message.
   */
  Error(ErrorCode code, const std::string &message)
      : m_Code(code), m_Message(message) {}

  /**
   * @brief Construct an error with a code and message (move).
   * @param code The error code.
   * @param message Descriptive error message (moved).
   */
  Error(ErrorCode code, std::string &&message)
      : m_Code(code), m_Message(std::move(message)) {}

  /**
   * @brief Get the error code.
   * @return The error code.
   */
  ErrorCode Code() const { return m_Code; }

  /**
   * @brief Get the error message.
   * @return Reference to the error message (may be empty).
   */
  const std::string &Message() const { return m_Message; }

  /**
   * @brief Get a full description of the error.
   * @return String in format "ErrorCode: message" or just "ErrorCode".
   */
  std::string What() const {
    if (m_Message.empty()) {
      return ErrorCodeToString(m_Code);
    }
    return std::string(ErrorCodeToString(m_Code)) + ": " + m_Message;
  }

  /**
   * @brief Check if this represents an error (not Ok).
   * @return true if this is an error, false if Ok.
   */
  explicit operator bool() const { return m_Code != ErrorCode::Ok; }

private:
  ErrorCode m_Code;
  std::string m_Message;
};

/**
 * @brief Result type for operations that can fail.
 *
 * Holds either a value of type T on success, or an Error on failure.
 * Provides safe access to the value with error checking.
 *
 * @tparam T The type of the success value.
 */
template <typename T> class Result {
public:
  /**
   * @brief Construct a successful result with a value.
   * @param value The success value.
   */
  Result(const T &value) : m_Data(value) {}

  /**
   * @brief Construct a successful result with a value (move).
   * @param value The success value (moved).
   */
  Result(T &&value) : m_Data(std::move(value)) {}

  /**
   * @brief Construct a failed result with an error.
   * @param error The error.
   */
  Result(const Error &error) : m_Data(error) {}

  /**
   * @brief Construct a failed result with an error (move).
   * @param error The error (moved).
   */
  Result(Error &&error) : m_Data(std::move(error)) {}

  /**
   * @brief Construct a failed result with just an error code.
   * @param code The error code.
   */
  Result(ErrorCode code) : m_Data(Error(code)) {}

  /**
   * @brief Check if the result is successful.
   * @return true if result contains a value.
   */
  bool IsOk() const { return std::holds_alternative<T>(m_Data); }

  /**
   * @brief Check if the result is an error.
   * @return true if result contains an error.
   */
  bool IsError() const { return std::holds_alternative<Error>(m_Data); }

  /**
   * @brief Get the value (throws if error).
   * @return Reference to the value.
   * @throws std::bad_variant_access if result is an error.
   */
  T &Value() { return std::get<T>(m_Data); }

  /**
   * @brief Get the value (const, throws if error).
   * @return Const reference to the value.
   * @throws std::bad_variant_access if result is an error.
   */
  const T &Value() const { return std::get<T>(m_Data); }

  /**
   * @brief Get the value or a default if error.
   * @param defaultValue Value to return if this is an error.
   * @return The value or the default.
   */
  T ValueOr(const T &defaultValue) const {
    if (IsOk())
      return std::get<T>(m_Data);
    return defaultValue;
  }

  /**
   * @brief Get the error (throws if ok).
   * @return Const reference to the error.
   * @throws std::bad_variant_access if result is ok.
   */
  const Error &GetError() const { return std::get<Error>(m_Data); }

  /**
   * @brief Get error code (Ok if no error).
   * @return The error code, or ErrorCode::Ok if successful.
   */
  ErrorCode Code() const {
    if (IsError())
      return GetError().Code();
    return ErrorCode::Ok;
  }

  /**
   * @brief Implicit bool conversion.
   * @return true if successful.
   */
  explicit operator bool() const { return IsOk(); }

private:
  std::variant<T, Error> m_Data;
};

/**
 * @brief Specialization of Result for void operations.
 *
 * For operations that succeed with no return value, or fail with an error.
 */
template <> class Result<void> {
public:
  /**
   * @brief Construct a successful result.
   */
  Result() : m_Error(std::nullopt) {}

  /**
   * @brief Construct a failed result with an error.
   * @param error The error.
   */
  Result(const Error &error) : m_Error(error) {}

  /**
   * @brief Construct a failed result with an error (move).
   * @param error The error (moved).
   */
  Result(Error &&error) : m_Error(std::move(error)) {}

  /**
   * @brief Construct a failed result with just an error code.
   * @param code The error code.
   */
  Result(ErrorCode code) : m_Error(Error(code)) {}

  /**
   * @brief Check if successful.
   * @return true if no error.
   */
  bool IsOk() const { return !m_Error.has_value(); }

  /**
   * @brief Check if error.
   * @return true if there is an error.
   */
  bool IsError() const { return m_Error.has_value(); }

  /**
   * @brief Get the error (throws if ok).
   * @return Const reference to the error.
   * @throws std::bad_optional_access if result is ok.
   */
  const Error &GetError() const { return m_Error.value(); }

  /**
   * @brief Get error code (Ok if no error).
   * @return The error code, or ErrorCode::Ok if successful.
   */
  ErrorCode Code() const {
    if (IsError())
      return m_Error->Code();
    return ErrorCode::Ok;
  }

  /**
   * @brief Implicit bool conversion.
   * @return true if successful.
   */
  explicit operator bool() const { return IsOk(); }

private:
  std::optional<Error> m_Error;
};

/**
 * @brief Convenience alias for operations with no return value.
 *
 * Use Status for functions that either succeed or fail with no data to return.
 */
using Status = Result<void>;

/**
 * @brief Helper to create a successful Status.
 * @return A successful Status.
 */
inline Status Ok() { return Status(); }

/**
 * @brief Helper to create an error.
 * @param code The error code.
 * @param message Optional error message.
 * @return An Error object.
 */
inline Error MakeError(ErrorCode code, const std::string &message = "") {
  return Error(code, message);
}

} // namespace Orpheus
