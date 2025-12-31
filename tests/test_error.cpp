#include <catch2/catch_test_macros.hpp>

#include "include/Error.h"

using namespace Orpheus;

// ============================================================================
// ErrorCode Tests
// ============================================================================

TEST_CASE("ErrorCode values are distinct", "[Error]") {
  REQUIRE(ErrorCode::Ok != ErrorCode::EngineInitFailed);
  REQUIRE(ErrorCode::Ok != ErrorCode::FileNotFound);
  REQUIRE(ErrorCode::Ok != ErrorCode::EventNotFound);
}

TEST_CASE("ErrorCodeToString returns correct strings", "[Error]") {
  REQUIRE(std::string(ErrorCodeToString(ErrorCode::Ok)) == "Ok");
  REQUIRE(std::string(ErrorCodeToString(ErrorCode::FileNotFound)) ==
          "FileNotFound");
  REQUIRE(std::string(ErrorCodeToString(ErrorCode::JsonParseError)) ==
          "JsonParseError");
}

// ============================================================================
// Error Class Tests
// ============================================================================

TEST_CASE("Error default constructor creates Unknown error", "[Error]") {
  Error err;
  REQUIRE(err.Code() == ErrorCode::Unknown);
  REQUIRE(err.Message().empty());
}

TEST_CASE("Error with code only", "[Error]") {
  Error err(ErrorCode::FileNotFound);
  REQUIRE(err.Code() == ErrorCode::FileNotFound);
  REQUIRE(err.Message().empty());
  REQUIRE(err.What() == "FileNotFound");
}

TEST_CASE("Error with code and message", "[Error]") {
  Error err(ErrorCode::FileNotFound, "test.wav");
  REQUIRE(err.Code() == ErrorCode::FileNotFound);
  REQUIRE(err.Message() == "test.wav");
  REQUIRE(err.What() == "FileNotFound: test.wav");
}

TEST_CASE("Error bool conversion", "[Error]") {
  Error ok(ErrorCode::Ok);
  Error notOk(ErrorCode::InvalidParameter);

  REQUIRE_FALSE(static_cast<bool>(ok));
  REQUIRE(static_cast<bool>(notOk));
}

// ============================================================================
// Result<T> Tests
// ============================================================================

TEST_CASE("Result with value is Ok", "[Result]") {
  Result<int> r = 42;

  REQUIRE(r.IsOk());
  REQUIRE_FALSE(r.IsError());
  REQUIRE(r.Value() == 42);
  REQUIRE(r.Code() == ErrorCode::Ok);
}

TEST_CASE("Result with Error is not Ok", "[Result]") {
  Result<int> r = Error(ErrorCode::InvalidParameter, "bad value");

  REQUIRE_FALSE(r.IsOk());
  REQUIRE(r.IsError());
  REQUIRE(r.Code() == ErrorCode::InvalidParameter);
  REQUIRE(r.GetError().Message() == "bad value");
}

TEST_CASE("Result from ErrorCode", "[Result]") {
  Result<std::string> r = ErrorCode::FileNotFound;

  REQUIRE(r.IsError());
  REQUIRE(r.Code() == ErrorCode::FileNotFound);
}

TEST_CASE("Result ValueOr returns value on success", "[Result]") {
  Result<int> r = 100;
  REQUIRE(r.ValueOr(0) == 100);
}

TEST_CASE("Result ValueOr returns default on error", "[Result]") {
  Result<int> r = ErrorCode::InvalidParameter;
  REQUIRE(r.ValueOr(-1) == -1);
}

TEST_CASE("Result bool conversion", "[Result]") {
  Result<int> ok = 1;
  Result<int> err = ErrorCode::Unknown;

  REQUIRE(static_cast<bool>(ok));
  REQUIRE_FALSE(static_cast<bool>(err));
}

// ============================================================================
// Status (Result<void>) Tests
// ============================================================================

TEST_CASE("Status default is Ok", "[Status]") {
  Status s;
  REQUIRE(s.IsOk());
  REQUIRE_FALSE(s.IsError());
  REQUIRE(s.Code() == ErrorCode::Ok);
}

TEST_CASE("Status with Error", "[Status]") {
  Status s = Error(ErrorCode::EngineInitFailed, "init failed");

  REQUIRE_FALSE(s.IsOk());
  REQUIRE(s.IsError());
  REQUIRE(s.Code() == ErrorCode::EngineInitFailed);
}

TEST_CASE("Status from ErrorCode", "[Status]") {
  Status s = ErrorCode::JsonParseError;

  REQUIRE(s.IsError());
  REQUIRE(s.Code() == ErrorCode::JsonParseError);
}

TEST_CASE("Ok() helper creates success Status", "[Status]") {
  Status s = Ok();
  REQUIRE(s.IsOk());
}

TEST_CASE("MakeError helper creates Error", "[Status]") {
  Error e = MakeError(ErrorCode::BusNotFound, "Main");
  REQUIRE(e.Code() == ErrorCode::BusNotFound);
  REQUIRE(e.Message() == "Main");
}
