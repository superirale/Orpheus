#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "include/Voice.h"

using namespace Orpheus;

// ============================================================================
// VoiceState Tests
// ============================================================================

TEST_CASE("VoiceState enum values", "[Voice]") {
  VoiceState real = VoiceState::Real;
  VoiceState virt = VoiceState::Virtual;
  VoiceState stopped = VoiceState::Stopped;

  REQUIRE(real != virt);
  REQUIRE(virt != stopped);
  REQUIRE(real != stopped);
}

// ============================================================================
// StealBehavior Tests
// ============================================================================

TEST_CASE("StealBehavior enum values", "[Voice]") {
  StealBehavior oldest = StealBehavior::Oldest;
  StealBehavior furthest = StealBehavior::Furthest;
  StealBehavior quietest = StealBehavior::Quietest;
  StealBehavior none = StealBehavior::None;

  REQUIRE(oldest != furthest);
  REQUIRE(quietest != none);
}

// ============================================================================
// Voice Tests
// ============================================================================

TEST_CASE("Voice default values", "[Voice]") {
  Voice v;
  REQUIRE(v.id == 0);
  REQUIRE(v.eventName.empty());
  REQUIRE(v.handle == 0);
  REQUIRE(v.state == VoiceState::Stopped);
  REQUIRE(v.priority == 128);
  REQUIRE(v.volume == 1.0f);
}

TEST_CASE("Voice state helpers", "[Voice]") {
  Voice v;
  v.state = VoiceState::Real;
  REQUIRE(v.IsReal());
  REQUIRE_FALSE(v.IsVirtual());
  REQUIRE_FALSE(v.IsStopped());

  v.state = VoiceState::Virtual;
  REQUIRE_FALSE(v.IsReal());
  REQUIRE(v.IsVirtual());
  REQUIRE_FALSE(v.IsStopped());

  v.state = VoiceState::Stopped;
  REQUIRE_FALSE(v.IsReal());
  REQUIRE_FALSE(v.IsVirtual());
  REQUIRE(v.IsStopped());
}

TEST_CASE("Voice UpdateAudibility calculation", "[Voice]") {
  Voice v;
  v.position = {10.0f, 0.0f, 0.0f};
  v.maxDistance = 100.0f;
  v.volume = 1.0f;

  Vector3 listener{0.0f, 0.0f, 0.0f};
  v.UpdateAudibility(listener);

  // Distance is 10, maxDistance is 100
  // distAtten = 1.0 - (10/100) = 0.9
  // audibility = 1.0 * 0.9 = 0.9
  REQUIRE(v.audibility == Catch::Approx(0.9f));
}

TEST_CASE("Voice GetDistance calculation", "[Voice]") {
  Voice v;
  v.position = {3.0f, 4.0f, 0.0f};

  Vector3 listener{0.0f, 0.0f, 0.0f};
  float dist = v.GetDistance(listener);

  // sqrt(3^2 + 4^2) = 5
  REQUIRE(dist == Catch::Approx(5.0f));
}

TEST_CASE("Voice audibility at max distance", "[Voice]") {
  Voice v;
  v.position = {100.0f, 0.0f, 0.0f};
  v.maxDistance = 100.0f;
  v.volume = 1.0f;

  Vector3 listener{0.0f, 0.0f, 0.0f};
  v.UpdateAudibility(listener);

  REQUIRE(v.audibility == Catch::Approx(0.0f));
}

TEST_CASE("Voice audibility beyond max distance", "[Voice]") {
  Voice v;
  v.position = {150.0f, 0.0f, 0.0f};
  v.maxDistance = 100.0f;
  v.volume = 1.0f;

  Vector3 listener{0.0f, 0.0f, 0.0f};
  v.UpdateAudibility(listener);

  // Should be clamped to 0
  REQUIRE(v.audibility == Catch::Approx(0.0f));
}
