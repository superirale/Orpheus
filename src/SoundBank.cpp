#include "../include/SoundBank.h"

namespace Orpheus {

Status SoundBank::LoadFromJsonFile(const std::string &jsonPath) {
  std::ifstream file(jsonPath);
  if (!file.is_open()) {
    return Error(ErrorCode::FileNotFound,
                 "Failed to open JSON file: " + jsonPath);
  }

  try {
    nlohmann::json j;
    file >> j;

    if (j.is_array()) {
      for (const auto &eventJson : j) {
        auto result = RegisterEventFromJson(eventJson.dump());
        if (result.IsError()) {
          return result;
        }
      }
    }
    return Ok();
  } catch (const std::exception &e) {
    return Error(ErrorCode::JsonParseError, e.what());
  }
}

Status SoundBank::RegisterEventFromJson(const std::string &jsonString) {
  try {
    nlohmann::json j = nlohmann::json::parse(jsonString);

    EventDescriptor ed;
    ed.name = j.value("name", "");
    ed.path = j.value("sound", "");
    ed.bus = j.value("bus", "Master");

    if (j.contains("volume") && j["volume"].is_array() &&
        j["volume"].size() >= 2) {
      ed.volumeMin = j["volume"][0].get<float>();
      ed.volumeMax = j["volume"][1].get<float>();
    } else if (j.contains("volume") && j["volume"].is_number()) {
      ed.volumeMin = ed.volumeMax = j["volume"].get<float>();
    }

    if (j.contains("pitch") && j["pitch"].is_array() &&
        j["pitch"].size() >= 2) {
      ed.pitchMin = j["pitch"][0].get<float>();
      ed.pitchMax = j["pitch"][1].get<float>();
    } else if (j.contains("pitch") && j["pitch"].is_number()) {
      ed.pitchMin = ed.pitchMax = j["pitch"].get<float>();
    }

    ed.stream = j.value("stream", false);
    ed.priority = static_cast<uint8_t>(j.value("priority", 128));
    ed.maxDistance = j.value("maxDistance", 100.0f);

    if (j.contains("parameters") && j["parameters"].is_object()) {
      for (auto &[key, value] : j["parameters"].items()) {
        ed.parameters[key] = value.get<std::string>();
      }
    }

    if (ed.name.empty()) {
      return Error(ErrorCode::InvalidFormat, "Event missing 'name' field");
    }

    RegisterEvent(ed);
    return Ok();
  } catch (const std::exception &e) {
    return Error(ErrorCode::JsonParseError, e.what());
  }
}

void SoundBank::RegisterEvent(const EventDescriptor &ed) {
  events[ed.name] = ed;
}

Result<EventDescriptor> SoundBank::FindEvent(const std::string &name) {
  auto it = events.find(name);
  if (it == events.end()) {
    return Error(ErrorCode::EventNotFound, "Event not found: " + name);
  }
  return it->second;
}

} // namespace Orpheus
