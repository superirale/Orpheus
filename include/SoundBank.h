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
  std::unordered_map<std::string, std::string> parameters;
};

class SoundBank {
public:
  // Load all events from a JSON file containing an array of event objects
  bool loadFromJsonFile(const std::string &jsonPath) {
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
          registerEventFromJson(eventJson.dump());
        }
      }
      return true;
    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << "\n";
      return false;
    }
  }

  // Register event from a JSON string
  // Format:
  // {
  //   "name": "explosion",
  //   "sound": "assets/sfx/explosion.wav",
  //   "bus": "SFX",
  //   "volume": [0.8, 1.0],
  //   "pitch": [0.9, 1.1],
  //   "parameters": { "distance": "attenuation" }
  // }
  bool registerEventFromJson(const std::string &jsonString) {
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

      registerEvent(ed);
      return true;
    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << "\n";
      return false;
    }
  }

  void registerEvent(const EventDescriptor &ed) { events[ed.name] = ed; }

  bool findEvent(const std::string &name, EventDescriptor &out) {
    auto it = events.find(name);
    if (it == events.end())
      return false;
    out = it->second;
    return true;
  }

private:
  std::unordered_map<std::string, EventDescriptor> events;
};
