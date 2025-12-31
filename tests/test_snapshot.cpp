#include <catch2/catch_test_macros.hpp>

#include "include/Snapshot.h"

using namespace Orpheus;

// ============================================================================
// BusState Tests
// ============================================================================

TEST_CASE("BusState default volume", "[Snapshot]") {
  BusState state;
  REQUIRE(state.volume == 1.0f);
}

TEST_CASE("BusState custom volume", "[Snapshot]") {
  BusState state{0.5f};
  REQUIRE(state.volume == 0.5f);
}

// ============================================================================
// ReverbBusState Tests
// ============================================================================

TEST_CASE("ReverbBusState default values", "[Snapshot]") {
  ReverbBusState state;
  REQUIRE(state.wet == 0.5f);
  REQUIRE(state.roomSize == 0.5f);
  REQUIRE(state.damp == 0.5f);
  REQUIRE(state.width == 1.0f);
}

// ============================================================================
// Snapshot Tests
// ============================================================================

TEST_CASE("Snapshot bus state management", "[Snapshot]") {
  Snapshot snap;

  snap.SetBusState("Music", BusState{0.8f});
  snap.SetBusState("SFX", BusState{0.5f});

  const auto &states = snap.GetStates();
  REQUIRE(states.size() == 2);
  REQUIRE(states.at("Music").volume == 0.8f);
  REQUIRE(states.at("SFX").volume == 0.5f);
}

TEST_CASE("Snapshot reverb state management", "[Snapshot]") {
  Snapshot snap;

  ReverbBusState reverbState{0.7f, 0.8f, 0.3f, 1.0f};
  snap.SetReverbState("CaveReverb", reverbState);

  REQUIRE(snap.HasReverbState("CaveReverb"));
  REQUIRE_FALSE(snap.HasReverbState("Unknown"));

  const auto &states = snap.GetReverbStates();
  REQUIRE(states.size() == 1);
  REQUIRE(states.at("CaveReverb").wet == 0.7f);
}

TEST_CASE("Snapshot empty initially", "[Snapshot]") {
  Snapshot snap;
  REQUIRE(snap.GetStates().empty());
  REQUIRE(snap.GetReverbStates().empty());
}
