#pragma once

#include "AudioZone.h"
#include "Bus.h"
#include "Event.h"
#include "Parameter.h"
#include "SoundBank.h"
#include "Types.h"

class AudioManager {
public:
  AudioManager() : m_Engine(), m_Event(m_Engine, m_Bank) {}
  ~AudioManager() { shutdown(); }

  bool init() {
    SoLoud::result r = m_Engine.init();
    if (r != SoLoud::SO_NO_ERROR) {
      std::cerr << "SoLoud init failed: " << r << "\n";
      return false;
    }
    // Create default buses
    m_Buses.emplace("Master", std::make_shared<Bus>(m_Engine, "Master"));
    m_Buses.emplace("SFX", std::make_shared<Bus>(m_Engine, "SFX"));
    m_Buses.emplace("Music", std::make_shared<Bus>(m_Engine, "Music"));

    return true;
  }

  void shutdown() { m_Engine.deinit(); }

  void update(float dt, AudioData &data) {
    (void)dt;
    // Poll parameters, update filters, free finished voices, etc.

    m_Engine.set3dListenerParameters(data.listenerPos.x, data.listenerPos.y,
                                     data.listenerPos.z, 0, 0, 0, 0, 1, 0);
    for (auto &zone : m_Zones) {
      zone->update(data.listenerPos);
    }

    m_Engine.update3dAudio();
  }

  AudioHandle playEvent(const std::string &name) { return m_Event.play(name); }

  void registerEvent(const EventDescriptor &ed) { m_Bank.registerEvent(ed); }

  // Register event from JSON string
  // Format: {"name": "explosion", "sound": "assets/sfx/explosion.wav", ...}
  bool registerEvent(const std::string &jsonString) {
    return m_Bank.registerEventFromJson(jsonString);
  }

  // Load events from a JSON file
  bool loadEventsFromFile(const std::string &jsonPath) {
    return m_Bank.loadFromJsonFile(jsonPath);
  }

  // Example API to expose parameters
  void setGlobalParameter(const std::string &name, float value) {
    std::lock_guard<std::mutex> lock(m_ParamMutex);
    m_Parameters[name].Set(value);
  }

  void addAudioZone(const std::string &soundPath, const Vector3 &pos,
                    float inner, float outer, bool stream = true) {
    m_Zones.emplace_back(std::make_shared<AudioZone>(m_Engine, soundPath, pos,
                                                     inner, outer, stream));
  }

  Parameter *getParam(const std::string &name) {
    std::lock_guard<std::mutex> lock(m_ParamMutex);
    return &m_Parameters[name];
  }

  SoLoud::Soloud &engine() { return m_Engine; }

private:
  SoLoud::Soloud m_Engine;
  SoundBank m_Bank;
  AudioEvent m_Event;
  std::unordered_map<std::string, std::shared_ptr<Bus>> m_Buses;
  std::vector<std::shared_ptr<AudioZone>> m_Zones;
  std::unordered_map<std::string, Parameter> m_Parameters;
  std::mutex m_ParamMutex;
};
