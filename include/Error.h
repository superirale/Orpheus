#pragma once

#include <string>
#include <variant>

namespace Orpheus {

/// Error codes for Orpheus operations
enum class ErrorCode {
  Ok = 0,

  // Initialization errors
  EngineInitFailed,
  AlreadyInitialized,
  NotInitialized,

  // Resource errors
  FileNotFound,
  InvalidPath,
  JsonParseError,
  InvalidFormat,

  // Playback errors
  EventNotFound,
  VoiceAllocationFailed,
  InvalidHandle,
  PlaybackFailed,

  // Bus/Zone errors
  BusNotFound,
  BusAlreadyExists,
  ReverbBusNotFound,
  ReverbBusInitFailed,
  SnapshotNotFound,
  ZoneNotFound,
  ListenerNotFound,

  // Parameter errors
  InvalidParameter,
  OutOfRange,

  // General
  Unknown
};

/// Human-readable error code names
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

/// Error class with code and message
class Error {
public:
  Error() : m_Code(ErrorCode::Unknown) {}

  Error(ErrorCode code) : m_Code(code) {}

  Error(ErrorCode code, const std::string &message)
      : m_Code(code), m_Message(message) {}

  Error(ErrorCode code, std::string &&message)
      : m_Code(code), m_Message(std::move(message)) {}

  /// Get the error code
  ErrorCode Code() const { return m_Code; }

  /// Get the error message (may be empty)
  const std::string &Message() const { return m_Message; }

  /// Get a full description: "ErrorCode: message" or just "ErrorCode"
  std::string What() const {
    if (m_Message.empty()) {
      return ErrorCodeToString(m_Code);
    }
    return std::string(ErrorCodeToString(m_Code)) + ": " + m_Message;
  }

  /// Check if this is an error (not Ok)
  explicit operator bool() const { return m_Code != ErrorCode::Ok; }

private:
  ErrorCode m_Code;
  std::string m_Message;
};

/// Result type for operations that can fail
/// Holds either a value T or an Error
template <typename T> class Result {
public:
  /// Construct with a value (success)
  Result(const T &value) : m_Data(value) {}
  Result(T &&value) : m_Data(std::move(value)) {}

  /// Construct with an error (failure)
  Result(const Error &error) : m_Data(error) {}
  Result(Error &&error) : m_Data(std::move(error)) {}

  /// Construct with just an error code
  Result(ErrorCode code) : m_Data(Error(code)) {}

  /// Check if the result is successful
  bool IsOk() const { return std::holds_alternative<T>(m_Data); }

  /// Check if the result is an error
  bool IsError() const { return std::holds_alternative<Error>(m_Data); }

  /// Get the value (throws if error)
  T &Value() { return std::get<T>(m_Data); }
  const T &Value() const { return std::get<T>(m_Data); }

  /// Get the value or a default
  T ValueOr(const T &defaultValue) const {
    if (IsOk())
      return std::get<T>(m_Data);
    return defaultValue;
  }

  /// Get the error (throws if ok)
  const Error &GetError() const { return std::get<Error>(m_Data); }

  /// Get error code (Ok if no error)
  ErrorCode Code() const {
    if (IsError())
      return GetError().Code();
    return ErrorCode::Ok;
  }

  /// Implicit bool conversion (true if success)
  explicit operator bool() const { return IsOk(); }

private:
  std::variant<T, Error> m_Data;
};

/// Specialization for void (operations with no return value)
template <> class Result<void> {
public:
  /// Construct success
  Result() : m_Error(std::nullopt) {}

  /// Construct with an error
  Result(const Error &error) : m_Error(error) {}
  Result(Error &&error) : m_Error(std::move(error)) {}

  /// Construct with just an error code
  Result(ErrorCode code) : m_Error(Error(code)) {}

  /// Check if successful
  bool IsOk() const { return !m_Error.has_value(); }

  /// Check if error
  bool IsError() const { return m_Error.has_value(); }

  /// Get the error (throws if ok)
  const Error &GetError() const { return m_Error.value(); }

  /// Get error code (Ok if no error)
  ErrorCode Code() const {
    if (IsError())
      return m_Error->Code();
    return ErrorCode::Ok;
  }

  /// Implicit bool conversion (true if success)
  explicit operator bool() const { return IsOk(); }

private:
  std::optional<Error> m_Error;
};

/// Convenience alias for operations with no return value
using Status = Result<void>;

/// Helper to create a successful Status
inline Status Ok() { return Status(); }

/// Helper to create an error
inline Error MakeError(ErrorCode code, const std::string &message = "") {
  return Error(code, message);
}

} // namespace Orpheus
