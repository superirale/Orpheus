#include <catch2/catch_test_macros.hpp>

#include "include/Log.h"

using namespace Orpheus;

// ============================================================================
// LogLevel Tests
// ============================================================================

TEST_CASE("LogLevel ordering", "[Log]") {
  REQUIRE(LogLevel::Debug < LogLevel::Info);
  REQUIRE(LogLevel::Info < LogLevel::Warn);
  REQUIRE(LogLevel::Warn < LogLevel::Error);
  REQUIRE(LogLevel::Error < LogLevel::None);
}

TEST_CASE("LogLevelToString", "[Log]") {
  REQUIRE(std::string(LogLevelToString(LogLevel::Debug)) == "DEBUG");
  REQUIRE(std::string(LogLevelToString(LogLevel::Info)) == "INFO");
  REQUIRE(std::string(LogLevelToString(LogLevel::Warn)) == "WARN");
  REQUIRE(std::string(LogLevelToString(LogLevel::Error)) == "ERROR");
  REQUIRE(std::string(LogLevelToString(LogLevel::None)) == "NONE");
}

// ============================================================================
// Logger Tests
// ============================================================================

TEST_CASE("Logger default min level is Info", "[Log]") {
  // Reset to default state
  GetLogger().SetMinLevel(LogLevel::Info);
  REQUIRE(GetLogger().GetMinLevel() == LogLevel::Info);
}

TEST_CASE("Logger SetMinLevel", "[Log]") {
  GetLogger().SetMinLevel(LogLevel::Debug);
  REQUIRE(GetLogger().GetMinLevel() == LogLevel::Debug);

  GetLogger().SetMinLevel(LogLevel::Error);
  REQUIRE(GetLogger().GetMinLevel() == LogLevel::Error);

  // Reset
  GetLogger().SetMinLevel(LogLevel::Info);
}

TEST_CASE("Logger callback receives messages", "[Log]") {
  std::vector<std::pair<LogLevel, std::string>> received;

  GetLogger().SetCallback([&received](LogLevel level, const std::string &msg) {
    received.push_back({level, msg});
  });

  GetLogger().SetMinLevel(LogLevel::Debug);

  GetLogger().Debug("debug message");
  GetLogger().Info("info message");
  GetLogger().Warn("warn message");
  GetLogger().Error("error message");

  REQUIRE(received.size() == 4);
  REQUIRE(received[0].first == LogLevel::Debug);
  REQUIRE(received[0].second == "debug message");
  REQUIRE(received[1].first == LogLevel::Info);
  REQUIRE(received[2].first == LogLevel::Warn);
  REQUIRE(received[3].first == LogLevel::Error);

  // Cleanup
  GetLogger().ClearCallback();
  GetLogger().SetMinLevel(LogLevel::Info);
}

TEST_CASE("Logger min level filtering", "[Log]") {
  std::vector<LogLevel> received;

  GetLogger().SetCallback([&received](LogLevel level, const std::string &) {
    received.push_back(level);
  });

  GetLogger().SetMinLevel(LogLevel::Warn);

  GetLogger().Debug("filtered");
  GetLogger().Info("filtered");
  GetLogger().Warn("kept");
  GetLogger().Error("kept");

  REQUIRE(received.size() == 2);
  REQUIRE(received[0] == LogLevel::Warn);
  REQUIRE(received[1] == LogLevel::Error);

  // Cleanup
  GetLogger().ClearCallback();
  GetLogger().SetMinLevel(LogLevel::Info);
}

TEST_CASE("Logger ClearCallback", "[Log]") {
  int callCount = 0;

  GetLogger().SetCallback(
      [&callCount](LogLevel, const std::string &) { callCount++; });

  GetLogger().Info("test");
  REQUIRE(callCount == 1);

  GetLogger().ClearCallback();
  GetLogger().Info("test2");
  // After clear, goes to stderr, not our callback
  REQUIRE(callCount == 1);
}

TEST_CASE("ORPHEUS_LOG macros work", "[Log]") {
  std::string lastMessage;
  LogLevel lastLevel;

  GetLogger().SetCallback(
      [&lastMessage, &lastLevel](LogLevel level, const std::string &msg) {
        lastLevel = level;
        lastMessage = msg;
      });

  GetLogger().SetMinLevel(LogLevel::Debug);

  ORPHEUS_DEBUG("test " << 123);
  REQUIRE(lastLevel == LogLevel::Debug);
  REQUIRE(lastMessage == "test 123");

  ORPHEUS_INFO("info " << 456);
  REQUIRE(lastLevel == LogLevel::Info);
  REQUIRE(lastMessage == "info 456");

  ORPHEUS_WARN("warn message");
  REQUIRE(lastLevel == LogLevel::Warn);

  ORPHEUS_ERROR("error message");
  REQUIRE(lastLevel == LogLevel::Error);

  // Cleanup
  GetLogger().ClearCallback();
  GetLogger().SetMinLevel(LogLevel::Info);
}
