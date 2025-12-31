#include <catch2/catch_test_macros.hpp>

#include "include/SoundBank.h"

using namespace Orpheus;

// ============================================================================
// EventDescriptor Tests
// ============================================================================

TEST_CASE("EventDescriptor default values", "[SoundBank]") {
  EventDescriptor ed;
  REQUIRE(ed.name.empty());
  REQUIRE(ed.path.empty());
  REQUIRE(ed.bus.empty());
  REQUIRE(ed.volumeMin == 1.0f);
  REQUIRE(ed.volumeMax == 1.0f);
  REQUIRE(ed.pitchMin == 1.0f);
  REQUIRE(ed.pitchMax == 1.0f);
  REQUIRE(ed.stream == false);
  REQUIRE(ed.priority == 128);
  REQUIRE(ed.maxDistance == 100.0f);
}

// ============================================================================
// SoundBank Tests
// ============================================================================

TEST_CASE("SoundBank RegisterEvent and FindEvent", "[SoundBank]") {
  SoundBank bank;

  EventDescriptor ed;
  ed.name = "footstep";
  ed.path = "sounds/footstep.wav";
  ed.bus = "SFX";
  ed.volumeMin = 0.8f;

  bank.RegisterEvent(ed);

  auto result = bank.FindEvent("footstep");
  REQUIRE(result.IsOk());
  REQUIRE(result.Value().name == "footstep");
  REQUIRE(result.Value().path == "sounds/footstep.wav");
  REQUIRE(result.Value().bus == "SFX");
  REQUIRE(result.Value().volumeMin == 0.8f);
}

TEST_CASE("SoundBank FindEvent returns error for unknown event",
          "[SoundBank]") {
  SoundBank bank;

  auto result = bank.FindEvent("nonexistent");
  REQUIRE(result.IsError());
  REQUIRE(result.Code() == ErrorCode::EventNotFound);
}

TEST_CASE("SoundBank RegisterEventFromJson", "[SoundBank]") {
  SoundBank bank;

  std::string json = R"({
    "name": "explosion",
    "sound": "audio/explosion.wav",
    "bus": "SFX",
    "volume": [0.9, 1.0],
    "pitch": [0.95, 1.05],
    "stream": false,
    "priority": 200
  })";

  auto result = bank.RegisterEventFromJson(json);
  REQUIRE(result.IsOk());

  auto findResult = bank.FindEvent("explosion");
  REQUIRE(findResult.IsOk());
  REQUIRE(findResult.Value().path == "audio/explosion.wav");
  REQUIRE(findResult.Value().volumeMin == 0.9f);
  REQUIRE(findResult.Value().volumeMax == 1.0f);
  REQUIRE(findResult.Value().priority == 200);
}

TEST_CASE("SoundBank RegisterEventFromJson with single volume", "[SoundBank]") {
  SoundBank bank;

  std::string json = R"({
    "name": "beep",
    "sound": "beep.wav",
    "volume": 0.5
  })";

  auto result = bank.RegisterEventFromJson(json);
  REQUIRE(result.IsOk());

  auto findResult = bank.FindEvent("beep");
  REQUIRE(findResult.Value().volumeMin == 0.5f);
  REQUIRE(findResult.Value().volumeMax == 0.5f);
}

TEST_CASE("SoundBank RegisterEventFromJson missing name fails", "[SoundBank]") {
  SoundBank bank;

  std::string json = R"({
    "sound": "test.wav"
  })";

  auto result = bank.RegisterEventFromJson(json);
  REQUIRE(result.IsError());
  REQUIRE(result.Code() == ErrorCode::InvalidFormat);
}

TEST_CASE("SoundBank RegisterEventFromJson invalid JSON fails", "[SoundBank]") {
  SoundBank bank;

  std::string json = "{ invalid json }";

  auto result = bank.RegisterEventFromJson(json);
  REQUIRE(result.IsError());
  REQUIRE(result.Code() == ErrorCode::JsonParseError);
}

TEST_CASE("SoundBank multiple events", "[SoundBank]") {
  SoundBank bank;

  EventDescriptor e1, e2, e3;
  e1.name = "event1";
  e2.name = "event2";
  e3.name = "event3";

  bank.RegisterEvent(e1);
  bank.RegisterEvent(e2);
  bank.RegisterEvent(e3);

  REQUIRE(bank.FindEvent("event1").IsOk());
  REQUIRE(bank.FindEvent("event2").IsOk());
  REQUIRE(bank.FindEvent("event3").IsOk());
}
