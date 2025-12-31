#pragma once

#include "AudioZone.h"
#include "Bus.h"
#include "Event.h"
#include "Listener.h"
#include "MixZone.h"
#include "Parameter.h"
#include "ReverbBus.h"
#include "ReverbZone.h"
#include "Snapshot.h"
#include "SoundBank.h"
#include "Types.h"
#include "VoicePool.h"

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
    createBus("Master");
    createBus("SFX");
    createBus("Music");

    return true;
  }

  void shutdown() { m_Engine.deinit(); }

  void update(float dt) {
    for (auto &[_, bus] : m_Buses)
      bus->update(dt);

    // Get first listener position for voice pool and zones
    Vector3 listenerPos{0, 0, 0};
    for (auto &[id, listener] : m_Listeners) {
      if (!listener.active)
        continue;
      listenerPos = {listener.posX, listener.posY, listener.posZ};
      m_Engine.set3dListenerParameters(
          listener.posX, listener.posY, listener.posZ, listener.velX,
          listener.velY, listener.velZ, listener.forwardX, listener.forwardY,
          listener.forwardZ, listener.upX, listener.upY, listener.upZ);
      for (auto &zone : m_Zones) {
        zone->update(listenerPos);
      }
    }

    // Update voice pool (virtualization/promotion)
    m_VoicePool.update(dt, listenerPos);

    // Process voice state changes
    for (auto &voice : m_VoicePool.getVoices()) {
      if (voice.isStopped())
        continue;

      // Handle voices that need to start playing
      if (voice.isReal() && voice.handle == 0) {
        voice.handle = m_Event.play(voice.eventName);
        if (voice.handle != 0) {
          EventDescriptor ed;
          if (m_Bank.findEvent(voice.eventName, ed)) {
            const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
            if (m_Buses.count(busName)) {
              m_Buses[busName]->addHandle(m_Engine, voice.handle);
            }
          }
        }
      }
      // Handle voices that became virtual
      else if (voice.isVirtual() && voice.handle != 0) {
        m_Engine.stop(voice.handle);
        voice.handle = 0;
      }
    }

    // Update mix zones and apply highest priority active snapshot
    updateMixZones(listenerPos);

    // Update reverb zones (calculate zone influence on reverb buses)
    updateReverbZones(listenerPos);

    m_Engine.update3dAudio();
  }

  // Play event with optional 3D position (uses voice pool)
  VoiceID playEvent(const std::string &name, Vector3 position = {0, 0, 0}) {
    EventDescriptor ed;
    if (!m_Bank.findEvent(name, ed)) {
      std::cerr << "Event not found: " << name << "\n";
      return 0;
    }

    // Allocate voice in pool
    Voice *voice =
        m_VoicePool.allocateVoice(name, ed.priority, position, ed.maxDistance);
    if (!voice)
      return 0;

    voice->volume = ed.volumeMin;

    // Try to make it real (may steal another voice)
    if (m_VoicePool.makeReal(voice)) {
      voice->handle = m_Event.play(name);
      if (voice->handle != 0) {
        const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
        if (m_Buses.count(busName)) {
          m_Buses[busName]->addHandle(m_Engine, voice->handle);
        }
      }
    }

    return voice->id;
  }

  // Legacy: play event without voice pool (returns AudioHandle)
  AudioHandle playEventDirect(const std::string &name) {
    EventDescriptor ed;
    if (!m_Bank.findEvent(name, ed)) {
      return m_Event.play(name);
    }
    AudioHandle h = m_Event.play(name);
    if (h != 0) {
      const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
      if (m_Buses.count(busName)) {
        m_Buses[busName]->addHandle(m_Engine, h);
      }
    }
    return h;
  }

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

  // Add audio zone without snapshot binding
  // NOTE: Uses playEventDirect() because AudioZone needs actual SoLoud handles
  // to control playback (stop, volume). playEvent() returns VoiceIDs which are
  // different from audio handles.
  void addAudioZone(const std::string &eventName, const Vector3 &pos,
                    float inner, float outer) {
    m_Zones.emplace_back(std::make_shared<AudioZone>(
        eventName, pos, inner, outer,
        // PlayEvent callback - must use Direct to get actual audio handle
        [this](const std::string &name) { return this->playEventDirect(name); },
        // SetVolume callback
        [this](AudioHandle h, float v) { m_Engine.setVolume(h, v); },
        // Stop callback
        [this](AudioHandle h) { m_Engine.stop(h); },
        // IsValid callback
        [this](AudioHandle h) { return m_Engine.isValidVoiceHandle(h); }));
  }

  // Add audio zone with snapshot binding
  // When the listener enters the zone, the bound snapshot is applied.
  // When the listener exits, bus volumes reset to default.
  // Example: audio.addAudioZone("waterfall", {100, 0, 0}, 10.0f, 20.0f,
  // "Underwater");
  void addAudioZone(const std::string &eventName, const Vector3 &pos,
                    float inner, float outer, const std::string &snapshotName,
                    float fadeIn = 0.5f, float fadeOut = 0.5f) {
    m_Zones.emplace_back(std::make_shared<AudioZone>(
        eventName, pos, inner, outer,
        // PlayEvent callback - must use Direct to get actual audio handle
        [this](const std::string &name) { return this->playEventDirect(name); },
        // SetVolume callback
        [this](AudioHandle h, float v) { m_Engine.setVolume(h, v); },
        // Stop callback
        [this](AudioHandle h) { m_Engine.stop(h); },
        // IsValid callback
        [this](AudioHandle h) { return m_Engine.isValidVoiceHandle(h); },
        // Snapshot name
        snapshotName,
        // ApplySnapshot callback
        [this](const std::string &snap, float fade) {
          this->applySnapshot(snap, fade);
        },
        // RevertSnapshot callback
        [this](float fade) { this->resetBusVolumes(fade); },
        // Fade times
        fadeIn, fadeOut));
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

  // ---------------- Bus API ----------------
  void createBus(const std::string &name) {
    m_Buses[name] = std::make_shared<Bus>(name);
  }

  std::shared_ptr<Bus> getBus(const std::string &name) { return m_Buses[name]; }

  // ---------------- Snapshot API ----------------
  void createSnapshot(const std::string &name) {
    m_Snapshots[name] = Snapshot();
  }

  void setSnapshotBusVolume(const std::string &snap, const std::string &bus,
                            float volume) {
    m_Snapshots[snap].setBusState(bus, BusState{volume});
  }

  void applySnapshot(const std::string &name, float fadeSeconds = 0.3f) {
    const auto &states = m_Snapshots[name].getStates();
    for (const auto &[busName, state] : states) {
      if (m_Buses.count(busName))
        m_Buses[busName]->setTargetVolume(state.volume, fadeSeconds);
    }
  }

  // Reset all bus volumes to 1.0
  void resetBusVolumes(float fadeSeconds = 0.3f) {
    for (auto &[name, bus] : m_Buses) {
      bus->setTargetVolume(1.0f, fadeSeconds);
    }
  }

  // Reset a specific event's bus to its EventDescriptor.volumeMin
  void resetEventVolume(const std::string &eventName,
                        float fadeSeconds = 0.3f) {
    EventDescriptor ed;
    if (m_Bank.findEvent(eventName, ed)) {
      const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
      if (m_Buses.count(busName)) {
        m_Buses[busName]->setTargetVolume(ed.volumeMin, fadeSeconds);
      }
    }
  }

  // ---------------- Voice Pool API ----------------
  void setMaxVoices(uint32_t maxReal) { m_VoicePool.setMaxVoices(maxReal); }
  uint32_t getMaxVoices() const { return m_VoicePool.getMaxVoices(); }

  void setStealBehavior(StealBehavior behavior) {
    m_VoicePool.setStealBehavior(behavior);
  }
  StealBehavior getStealBehavior() const {
    return m_VoicePool.getStealBehavior();
  }

  uint32_t getActiveVoiceCount() const {
    return m_VoicePool.getActiveVoiceCount();
  }
  uint32_t getRealVoiceCount() const { return m_VoicePool.getRealVoiceCount(); }
  uint32_t getVirtualVoiceCount() const {
    return m_VoicePool.getVirtualVoiceCount();
  }

  // ---------------- Mix Zone API ----------------
  void addMixZone(const std::string &name, const std::string &snapshotName,
                  const Vector3 &pos, float inner, float outer,
                  uint8_t priority = 128, float fadeIn = 0.5f,
                  float fadeOut = 0.5f) {
    m_MixZones.emplace_back(std::make_shared<MixZone>(
        name, snapshotName, pos, inner, outer, priority, fadeIn, fadeOut));
  }

  void removeMixZone(const std::string &name) {
    m_MixZones.erase(
        std::remove_if(m_MixZones.begin(), m_MixZones.end(),
                       [&name](const auto &z) { return z->getName() == name; }),
        m_MixZones.end());
  }

  void setZoneEnterCallback(ZoneEnterCallback cb) { m_ZoneEnterCallback = cb; }
  void setZoneExitCallback(ZoneExitCallback cb) { m_ZoneExitCallback = cb; }

  const std::string &getActiveMixZone() const { return m_ActiveMixZone; }

  SoLoud::Soloud &engine() { return m_Engine; }

  // ---------------- Reverb Bus API ----------------

  // Create a reverb bus with initial parameters
  bool createReverbBus(const std::string &name, float roomSize = 0.5f,
                       float damp = 0.5f, float wet = 0.5f,
                       float width = 1.0f) {
    if (m_ReverbBuses.count(name) > 0) {
      return false; // Already exists
    }

    auto reverbBus = std::make_shared<ReverbBus>(name);
    reverbBus->setParams(wet, roomSize, damp, width);

    if (!reverbBus->init(m_Engine)) {
      std::cerr << "Failed to initialize reverb bus: " << name << "\n";
      return false;
    }

    m_ReverbBuses[name] = reverbBus;
    return true;
  }

  // Create a reverb bus from a preset
  bool createReverbBus(const std::string &name, ReverbPreset preset) {
    if (m_ReverbBuses.count(name) > 0) {
      return false;
    }

    auto reverbBus = std::make_shared<ReverbBus>(name);
    reverbBus->applyPreset(preset);

    if (!reverbBus->init(m_Engine)) {
      std::cerr << "Failed to initialize reverb bus: " << name << "\n";
      return false;
    }

    m_ReverbBuses[name] = reverbBus;
    return true;
  }

  // Get a reverb bus by name
  std::shared_ptr<ReverbBus> getReverbBus(const std::string &name) {
    auto it = m_ReverbBuses.find(name);
    return (it != m_ReverbBuses.end()) ? it->second : nullptr;
  }

  // Set reverb bus parameters with fade
  void setReverbParams(const std::string &name, float wet, float roomSize,
                       float damp, float fadeTime = 0.0f) {
    auto bus = getReverbBus(name);
    if (bus) {
      bus->setWet(wet, fadeTime);
      bus->setRoomSize(roomSize, fadeTime);
      bus->setDamp(damp, fadeTime);
    }
  }

  // Add a reverb zone
  void addReverbZone(const std::string &name, const std::string &reverbBusName,
                     const Vector3 &pos, float inner, float outer,
                     uint8_t priority = 128) {
    m_ReverbZones.emplace_back(std::make_shared<ReverbZone>(
        name, reverbBusName, pos, inner, outer, priority));
  }

  // Remove a reverb zone by name
  void removeReverbZone(const std::string &name) {
    m_ReverbZones.erase(
        std::remove_if(m_ReverbZones.begin(), m_ReverbZones.end(),
                       [&name](const auto &z) { return z->getName() == name; }),
        m_ReverbZones.end());
  }

  // Set snapshot reverb parameters
  void setSnapshotReverbParams(const std::string &snapshotName,
                               const std::string &reverbBusName, float wet,
                               float roomSize, float damp, float width = 1.0f) {
    m_Snapshots[snapshotName].setReverbState(
        reverbBusName, ReverbBusState{wet, roomSize, damp, width});
  }

  // Get active reverb zones
  std::vector<std::string> getActiveReverbZones() const {
    std::vector<std::string> active;
    for (const auto &zone : m_ReverbZones) {
      if (zone->isActive()) {
        active.push_back(zone->getName());
      }
    }
    return active;
  }

private:
  SoLoud::Soloud m_Engine;
  SoundBank m_Bank;
  AudioEvent m_Event;
  VoicePool m_VoicePool;
  std::unordered_map<std::string, std::shared_ptr<Bus>> m_Buses;
  std::vector<std::shared_ptr<AudioZone>> m_Zones;
  std::unordered_map<ListenerID, Listener> m_Listeners;
  ListenerID m_NextListenerID = 1;
  std::unordered_map<std::string, Parameter> m_Parameters;
  std::mutex m_ParamMutex;

  std::unordered_map<std::string, Snapshot> m_Snapshots;

  // Mix zones for snapshot binding
  std::vector<std::shared_ptr<MixZone>> m_MixZones;
  std::string m_ActiveMixZone; // Currently active zone name
  ZoneEnterCallback m_ZoneEnterCallback;
  ZoneExitCallback m_ZoneExitCallback;

  // Reverb system
  std::unordered_map<std::string, std::shared_ptr<ReverbBus>> m_ReverbBuses;
  std::vector<std::shared_ptr<ReverbZone>> m_ReverbZones;

  void updateMixZones(const Vector3 &listenerPos) {
    // Update all mix zones
    for (auto &zone : m_MixZones) {
      zone->update(listenerPos);
    }

    // Find highest priority active zone
    MixZone *bestZone = nullptr;
    for (auto &zone : m_MixZones) {
      if (!zone->isActive())
        continue;
      if (!bestZone || zone->getPriority() > bestZone->getPriority() ||
          (zone->getPriority() == bestZone->getPriority() &&
           zone->getBlendFactor() > bestZone->getBlendFactor())) {
        bestZone = zone.get();
      }
    }

    // Handle zone transitions
    std::string newActiveZone = bestZone ? bestZone->getName() : "";
    if (newActiveZone != m_ActiveMixZone) {
      // Exit previous zone
      if (!m_ActiveMixZone.empty()) {
        if (m_ZoneExitCallback) {
          m_ZoneExitCallback(m_ActiveMixZone);
        }
        // If exiting to no zone, reset volumes
        if (newActiveZone.empty()) {
          resetBusVolumes(0.5f);
        }
      }
      // Enter new zone
      if (!newActiveZone.empty() && m_ZoneEnterCallback) {
        m_ZoneEnterCallback(newActiveZone);
      }
      m_ActiveMixZone = newActiveZone;
    }

    // Apply snapshot with blend factor
    if (bestZone) {
      float blend = bestZone->getBlendFactor();
      applySnapshot(bestZone->getSnapshotName(),
                    blend * bestZone->getFadeInTime());
    }
  }

  // Update reverb zones based on listener position
  void updateReverbZones(const Vector3 &listenerPos) {
    // Track total influence per reverb bus
    std::unordered_map<std::string, float> busInfluence;

    // Update all reverb zones and accumulate influence
    for (auto &zone : m_ReverbZones) {
      float influence = zone->update(listenerPos);
      if (influence > 0.0f) {
        const std::string &busName = zone->getReverbBusName();
        // Use max influence (priority-based would be more complex)
        if (busInfluence.count(busName) == 0 ||
            influence > busInfluence[busName]) {
          busInfluence[busName] = influence;
        }
      }
    }

    // Apply accumulated influence to reverb buses
    for (auto &[busName, bus] : m_ReverbBuses) {
      float influence = 0.0f;
      if (busInfluence.count(busName) > 0) {
        influence = busInfluence[busName];
      }
      // Smooth fade the wet level based on zone influence
      float targetWet = influence * 0.8f; // Max 80% wet when fully in zone
      bus->setWet(targetWet, 0.1f);       // Small fade time for smoothness
    }
  }
};
