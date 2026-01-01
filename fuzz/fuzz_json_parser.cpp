/**
 * @file fuzz_json_parser.cpp
 * @brief Fuzz testing harness for JSON parsing in SoundBank.
 *
 * Uses libFuzzer to test robustness of JSON event parsing.
 * Build with: clang++ -fsanitize=fuzzer,address -O1 -g fuzz_json_parser.cpp
 */

#include "../include/SoundBank.h"

#include <cstdint>
#include <cstdlib>
#include <string>

using namespace Orpheus;

// libFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Prevent OOM on huge inputs
  if (size > 1024 * 1024) {
    return 0;
  }

  // Convert to string
  std::string input(reinterpret_cast<const char *>(data), size);

  SoundBank bank;

  // Fuzz single event registration from JSON string
  // This should handle malformed JSON gracefully
  (void)bank.RegisterEventFromJson(input);

  // Try to find a potentially registered event
  (void)bank.FindEvent("test");

  return 0;
}
