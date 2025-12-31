#pragma once

struct EventDescriptor {
  std::string name;
  std::string path;
  std::string bus;
  float volumeMin = 1.0f;
  float volumeMax = 1.0f;
  float pitchMin = 1.0f;
  float pitchMax = 1.0f;
  bool stream = false;
  uint8_t priority = 128;     // 0-255, higher = more important
  float maxDistance = 100.0f; // 3D attenuation distance
  std::unordered_map<std::string, std::string> parameters;
};

class SoundBank {
public:
  // Load all events from a JSON file containing an array of event objects
  bool LoadFromJsonFile(const std::string &jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
      std::cerr << "Failed to open JSON file: " << jsonPath << "\n";
      return false;
    }

    try {
      nlohmann::json j;
      file >> j;

      // Handle array of events
      if (j.is_array()) {
        for (const auto &eventJson : j) {
          RegisterEventFromJson(eventJson.dump());
        }
      }
      return true;
    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << "\n";
      return false;
    }
  }

  // Register event from a JSON string
  bool RegisterEventFromJson(const std::string &jsonString) {
    try {
      nlohmann::json j = nlohmann::json::parse(jsonString);

      EventDescriptor ed;
      ed.name = j.value("name", "");
      ed.path = j.value("sound", "");
      ed.bus = j.value("bus", "Master");

      // Parse volume range [min, max]
      if (j.contains("volume") && j["volume"].is_array() &&
          j["volume"].size() >= 2) {
        ed.volumeMin = j["volume"][0].get<float>();
        ed.volumeMax = j["volume"][1].get<float>();
      } else if (j.contains("volume") && j["volume"].is_number()) {
        ed.volumeMin = ed.volumeMax = j["volume"].get<float>();
      }

      // Parse pitch range [min, max]
      if (j.contains("pitch") && j["pitch"].is_array() &&
          j["pitch"].size() >= 2) {
        ed.pitchMin = j["pitch"][0].get<float>();
        ed.pitchMax = j["pitch"][1].get<float>();
      } else if (j.contains("pitch") && j["pitch"].is_number()) {
        ed.pitchMin = ed.pitchMax = j["pitch"].get<float>();
      }

      // Parse stream flag
      ed.stream = j.value("stream", false);

      // Parse priority (0-255, higher = more important)
      ed.priority = static_cast<uint8_t>(j.value("priority", 128));

      // Parse max distance for 3D attenuation
      ed.maxDistance = j.value("maxDistance", 100.0f);

      // Parse parameters map
      if (j.contains("parameters") && j["parameters"].is_object()) {
        for (auto &[key, value] : j["parameters"].items()) {
          ed.parameters[key] = value.get<std::string>();
        }
      }

      if (ed.name.empty()) {
        std::cerr << "Event missing 'name' field\n";
        return false;
      }

      RegisterEvent(ed);
      return true;
    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << "\n";
      return false;
    }
  }

  void RegisterEvent(const EventDescriptor &ed) { events[ed.name] = ed; }

  bool FindEvent(const std::string &name, EventDescriptor &out) {
    auto it = events.find(name);
    if (it == events.end())
      return false;
    out = it->second;
    return true;
  }

private:
  std::unordered_map<std::string, EventDescriptor> events;
};
