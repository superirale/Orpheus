#include <catch2/catch_test_macros.hpp>

#include "include/VoicePool.h"

using namespace Orpheus;

// ============================================================================
// VoicePool Tests
// ============================================================================

TEST_CASE("VoicePool default max voices", "[VoicePool]") {
  VoicePool pool;
  REQUIRE(pool.GetMaxVoices() == 32);
}

TEST_CASE("VoicePool constructor with max voices", "[VoicePool]") {
  VoicePool pool(16);
  REQUIRE(pool.GetMaxVoices() == 16);
}

TEST_CASE("VoicePool SetMaxVoices", "[VoicePool]") {
  VoicePool pool;
  pool.SetMaxVoices(64);
  REQUIRE(pool.GetMaxVoices() == 64);
}

TEST_CASE("VoicePool default steal behavior", "[VoicePool]") {
  VoicePool pool;
  REQUIRE(pool.GetStealBehavior() == StealBehavior::Quietest);
}

TEST_CASE("VoicePool SetStealBehavior", "[VoicePool]") {
  VoicePool pool;
  pool.SetStealBehavior(StealBehavior::Oldest);
  REQUIRE(pool.GetStealBehavior() == StealBehavior::Oldest);
}

TEST_CASE("VoicePool AllocateVoice", "[VoicePool]") {
  VoicePool pool(8);

  Voice *v = pool.AllocateVoice("test_event", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 100.0f});
  REQUIRE(v != nullptr);
  REQUIRE(v->eventName == "test_event");
  REQUIRE(v->priority == 128);
  REQUIRE(v->distanceSettings.maxDistance == 100.0f);
  REQUIRE(v->state == VoiceState::Virtual);
}

TEST_CASE("VoicePool voice IDs are unique", "[VoicePool]") {
  VoicePool pool;

  // Capture IDs immediately to avoid pointer invalidation from vector
  // reallocation
  VoiceID id1 = pool.AllocateVoice("event1", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f})->id;
  VoiceID id2 = pool.AllocateVoice("event2", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f})->id;
  VoiceID id3 = pool.AllocateVoice("event3", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f})->id;

  REQUIRE(id1 != id2);
  REQUIRE(id2 != id3);
  REQUIRE(id1 != id3);
}

TEST_CASE("VoicePool MakeReal changes state", "[VoicePool]") {
  VoicePool pool(8);

  Voice *v = pool.AllocateVoice("test", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 100.0f});
  REQUIRE(v->state == VoiceState::Virtual);

  bool success = pool.MakeReal(v);
  REQUIRE(success);
  REQUIRE(v->state == VoiceState::Real);
}

TEST_CASE("VoicePool MakeVirtual changes state", "[VoicePool]") {
  VoicePool pool(8);

  Voice *v = pool.AllocateVoice("test", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 100.0f});
  (void)pool.MakeReal(v);
  REQUIRE(v->state == VoiceState::Real);

  pool.MakeVirtual(v);
  REQUIRE(v->state == VoiceState::Virtual);
}

TEST_CASE("VoicePool StopVoice changes state", "[VoicePool]") {
  VoicePool pool(8);

  Voice *v = pool.AllocateVoice("test", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 100.0f});
  (void)pool.MakeReal(v);

  pool.StopVoice(v);
  REQUIRE(v->state == VoiceState::Stopped);
}

TEST_CASE("VoicePool GetActiveVoiceCount", "[VoicePool]") {
  VoicePool pool(8);

  REQUIRE(pool.GetActiveVoiceCount() == 0);

  (void)pool.AllocateVoice("e1", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f});
  (void)pool.AllocateVoice("e2", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f});

  REQUIRE(pool.GetActiveVoiceCount() == 2);
}

TEST_CASE("VoicePool GetRealVoiceCount and GetVirtualVoiceCount",
          "[VoicePool]") {
  VoicePool pool(8);

  // Allocate both voices first
  (void)pool.AllocateVoice("e1", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f});
  (void)pool.AllocateVoice("e2", 128, {0, 0, 0}, DistanceSettings{.maxDistance = 50.0f});

  // Get stable pointers via GetVoiceAt()
  REQUIRE(pool.GetVoiceCount() == 2);
  Voice *voice0 = pool.GetVoiceAt(0);
  Voice *voice1 = pool.GetVoiceAt(1);
  REQUIRE(voice0 != nullptr);
  REQUIRE(voice1 != nullptr);

  (void)pool.MakeReal(voice0);

  REQUIRE(pool.GetRealVoiceCount() == 1);
  REQUIRE(pool.GetVirtualVoiceCount() == 1);

  (void)pool.MakeReal(voice1);
  REQUIRE(pool.GetRealVoiceCount() == 2);
  REQUIRE(pool.GetVirtualVoiceCount() == 0);
}
