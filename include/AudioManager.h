#pragma once

#include "AudioZone.h"
#include "Bus.h"
#include "Event.h"
#include "Listener.h"
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

  void update(float dt) {
    (void)dt;
    // Update 3D audio for each listener
    for (auto &[id, listener] : m_Listeners) {
      if (!listener.active)
        continue;
      m_Engine.set3dListenerParameters(
          listener.posX, listener.posY, listener.posZ, listener.velX,
          listener.velY, listener.velZ, listener.forwardX, listener.forwardY,
          listener.forwardZ, listener.upX, listener.upY, listener.upZ);
      for (auto &zone : m_Zones) {
        zone->update({listener.posX, listener.posY, listener.posZ});
      }
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

  // Listener management
  ListenerID createListener() {
    ListenerID id = m_NextListenerID++;
    m_Listeners[id] = Listener{id};
    return id;
  }

  void destroyListener(ListenerID id) { m_Listeners.erase(id); }

  void setListenerPosition(ListenerID id, const Vector3 &pos) {
    if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
      it->second.posX = pos.x;
      it->second.posY = pos.y;
      it->second.posZ = pos.z;
    }
  }

  void setListenerPosition(ListenerID id, float x, float y, float z) {
    if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
      it->second.posX = x;
      it->second.posY = y;
      it->second.posZ = z;
    }
  }

  void setListenerVelocity(ListenerID id, const Vector3 &vel) {
    if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
      it->second.velX = vel.x;
      it->second.velY = vel.y;
      it->second.velZ = vel.z;
    }
  }

  void setListenerOrientation(ListenerID id, const Vector3 &forward,
                              const Vector3 &up) {
    if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
      it->second.forwardX = forward.x;
      it->second.forwardY = forward.y;
      it->second.forwardZ = forward.z;
      it->second.upX = up.x;
      it->second.upY = up.y;
      it->second.upZ = up.z;
    }
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
  std::unordered_map<ListenerID, Listener> m_Listeners;
  ListenerID m_NextListenerID = 1;
  std::unordered_map<std::string, Parameter> m_Parameters;
  std::mutex m_ParamMutex;
};
