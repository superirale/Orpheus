#include <catch2/catch_test_macros.hpp>

#include "include/Types.h"

using namespace Orpheus;

// ============================================================================
// Vector3 Tests
// ============================================================================

TEST_CASE("Vector3 default initialization", "[Types]") {
  Vector3 v{};
  REQUIRE(v.x == 0.0f);
  REQUIRE(v.y == 0.0f);
  REQUIRE(v.z == 0.0f);
}

TEST_CASE("Vector3 brace initialization", "[Types]") {
  Vector3 v{1.0f, 2.0f, 3.0f};
  REQUIRE(v.x == 1.0f);
  REQUIRE(v.y == 2.0f);
  REQUIRE(v.z == 3.0f);
}

// ============================================================================
// AudioHandle Tests
// ============================================================================

TEST_CASE("AudioHandle is a SoLoud handle type", "[Types]") {
  AudioHandle h = 0;
  REQUIRE(h == 0);

  AudioHandle h2 = 12345;
  REQUIRE(h2 == 12345);
}
