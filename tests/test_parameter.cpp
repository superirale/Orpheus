#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "include/Parameter.h"

using namespace Orpheus;

// ============================================================================
// Parameter Tests
// ============================================================================

TEST_CASE("Parameter default value", "[Parameter]") {
  Parameter p;
  REQUIRE(p.Get() == 0.0f);
}

TEST_CASE("Parameter constructor with value", "[Parameter]") {
  Parameter p(5.0f);
  REQUIRE(p.Get() == 5.0f);
}

TEST_CASE("Parameter Set and Get", "[Parameter]") {
  Parameter p;
  p.Set(3.14f);
  REQUIRE(p.Get() == Catch::Approx(3.14f));
}

TEST_CASE("Parameter callback binding", "[Parameter]") {
  Parameter p;
  float receivedValue = 0.0f;

  p.Bind([&receivedValue](float v) { receivedValue = v; });

  p.Set(42.0f);
  REQUIRE(receivedValue == 42.0f);
}

TEST_CASE("Parameter multiple callbacks", "[Parameter]") {
  Parameter p;
  int callCount = 0;

  p.Bind([&callCount](float) { callCount++; });
  p.Bind([&callCount](float) { callCount++; });

  p.Set(1.0f);
  REQUIRE(callCount == 2);
}

TEST_CASE("Parameter callbacks receive correct value", "[Parameter]") {
  Parameter p(100.0f);
  std::vector<float> received;

  p.Bind([&received](float v) { received.push_back(v); });
  p.Bind([&received](float v) { received.push_back(v * 2); });

  p.Set(10.0f);

  REQUIRE(received.size() == 2);
  REQUIRE(received[0] == 10.0f);
  REQUIRE(received[1] == 20.0f);
}
