#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

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
  bool LoadFromJsonFile(const std::string &jsonPath);
  bool RegisterEventFromJson(const std::string &jsonString);
  void RegisterEvent(const EventDescriptor &ed);
  bool FindEvent(const std::string &name, EventDescriptor &out);

private:
  std::unordered_map<std::string, EventDescriptor> events;
};

} // namespace Orpheus
