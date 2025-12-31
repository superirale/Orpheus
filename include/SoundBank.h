#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "Error.h"

namespace Orpheus {

struct EventDescriptor {
  std::string name;
  std::string path;
  std::string bus;
  float volumeMin = 1.0f;
  float volumeMax = 1.0f;
  float pitchMin = 1.0f;
  float pitchMax = 1.0f;
  bool stream = false;
  uint8_t priority = 128;
  float maxDistance = 100.0f;
  std::unordered_map<std::string, std::string> parameters;
};

class SoundBank {
public:
  Status LoadFromJsonFile(const std::string &jsonPath);
  Status RegisterEventFromJson(const std::string &jsonString);
  void RegisterEvent(const EventDescriptor &ed);
  Result<EventDescriptor> FindEvent(const std::string &name);

private:
  std::unordered_map<std::string, EventDescriptor> events;
};

} // namespace Orpheus
