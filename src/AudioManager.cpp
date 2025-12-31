#include "../include/AudioManager.h"

namespace Orpheus {

AudioManager::AudioManager() : m_Engine(), m_Event(m_Engine, m_Bank) {}

AudioManager::~AudioManager() { Shutdown(); }

Status AudioManager::Init() {
  SoLoud::result r = m_Engine.init();
  if (r != SoLoud::SO_NO_ERROR) {
    return Error(ErrorCode::EngineInitFailed,
                 "SoLoud init failed with code: " + std::to_string(r));
  }
  // Create default buses
  CreateBus("Master");
  CreateBus("SFX");
  CreateBus("Music");

  return Ok();
}

void AudioManager::Shutdown() { m_Engine.deinit(); }

void AudioManager::Update(float dt) {
  for (auto &[_, bus] : m_Buses)
    bus->Update(dt);

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
      zone->Update(listenerPos);
    }
  }

  // Update voice pool (virtualization/promotion)
  m_VoicePool.Update(dt, listenerPos);

  // Process voice state changes
  for (auto &voice : m_VoicePool.GetVoices()) {
    if (voice.IsStopped())
      continue;

    // Handle voices that need to start playing
    if (voice.IsReal() && voice.handle == 0) {
      voice.handle = m_Event.Play(voice.eventName);
      if (voice.handle != 0) {
        auto eventResult = m_Bank.FindEvent(voice.eventName);
        if (eventResult) {
          const auto &ed = eventResult.Value();
          const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
          if (m_Buses.count(busName)) {
            m_Buses[busName]->AddHandle(m_Engine, voice.handle);
          }
        }
      }
    }
    // Handle voices that became virtual
    else if (voice.IsVirtual() && voice.handle != 0) {
      m_Engine.stop(voice.handle);
      voice.handle = 0;
    }

    // Update occlusion for real voices
    if (voice.IsReal() && voice.handle != 0) {
      m_OcclusionProcessor.Update(voice, listenerPos, dt);
      m_OcclusionProcessor.ApplyDSP(m_Engine, voice);
    }
  }

  // Update mix zones and apply highest priority active snapshot
  UpdateMixZones(listenerPos);

  // Update reverb zones (calculate zone influence on reverb buses)
  UpdateReverbZones(listenerPos);

  m_Engine.update3dAudio();
}

Result<VoiceID> AudioManager::PlayEvent(const std::string &name,
                                        Vector3 position) {
  auto eventResult = m_Bank.FindEvent(name);
  if (eventResult.IsError()) {
    return eventResult.GetError();
  }
  const auto &ed = eventResult.Value();

  // Allocate voice in pool
  Voice *voice =
      m_VoicePool.AllocateVoice(name, ed.priority, position, ed.maxDistance);
  if (!voice)
    return Error(ErrorCode::VoiceAllocationFailed, "Failed to allocate voice");

  voice->volume = ed.volumeMin;

  // Try to make it real (may steal another voice)
  if (m_VoicePool.MakeReal(voice)) {
    voice->handle = m_Event.Play(name);
    if (voice->handle != 0) {
      const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
      if (m_Buses.count(busName)) {
        m_Buses[busName]->AddHandle(m_Engine, voice->handle);
      }
    }
  }

  return voice->id;
}

Result<AudioHandle> AudioManager::PlayEventDirect(const std::string &name) {
  auto eventResult = m_Bank.FindEvent(name);
  AudioHandle h = m_Event.Play(name);
  if (h == 0) {
    return Error(ErrorCode::PlaybackFailed, "Failed to play event: " + name);
  }
  if (eventResult) {
    const auto &ed = eventResult.Value();
    const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
    if (m_Buses.count(busName)) {
      m_Buses[busName]->AddHandle(m_Engine, h);
    }
  }
  return h;
}

void AudioManager::RegisterEvent(const EventDescriptor &ed) {
  m_Bank.RegisterEvent(ed);
}

Status AudioManager::RegisterEvent(const std::string &jsonString) {
  return m_Bank.RegisterEventFromJson(jsonString);
}

Status AudioManager::LoadEventsFromFile(const std::string &jsonPath) {
  return m_Bank.LoadFromJsonFile(jsonPath);
}

void AudioManager::SetGlobalParameter(const std::string &name, float value) {
  std::lock_guard<std::mutex> lock(m_ParamMutex);
  m_Parameters[name].Set(value);
}

Parameter *AudioManager::GetParam(const std::string &name) {
  std::lock_guard<std::mutex> lock(m_ParamMutex);
  return &m_Parameters[name];
}

void AudioManager::AddAudioZone(const std::string &eventName,
                                const Vector3 &pos, float inner, float outer) {
  m_Zones.emplace_back(std::make_shared<AudioZone>(
      eventName, pos, inner, outer,
      [this](const std::string &name) {
        auto result = this->PlayEventDirect(name);
        return result.ValueOr(0);
      },
      [this](AudioHandle h, float v) { m_Engine.setVolume(h, v); },
      [this](AudioHandle h) { m_Engine.stop(h); },
      [this](AudioHandle h) { return m_Engine.isValidVoiceHandle(h); }));
}

void AudioManager::AddAudioZone(const std::string &eventName,
                                const Vector3 &pos, float inner, float outer,
                                const std::string &snapshotName, float fadeIn,
                                float fadeOut) {
  m_Zones.emplace_back(std::make_shared<AudioZone>(
      eventName, pos, inner, outer,
      [this](const std::string &name) {
        auto result = this->PlayEventDirect(name);
        return result.ValueOr(0);
      },
      [this](AudioHandle h, float v) { m_Engine.setVolume(h, v); },
      [this](AudioHandle h) { m_Engine.stop(h); },
      [this](AudioHandle h) { return m_Engine.isValidVoiceHandle(h); },
      snapshotName,
      [this](const std::string &snap, float fade) {
        this->ApplySnapshot(snap, fade);
      },
      [this](float fade) { this->ResetBusVolumes(fade); }, fadeIn, fadeOut));
}

ListenerID AudioManager::CreateListener() {
  ListenerID id = m_NextListenerID++;
  m_Listeners[id] = Listener{id};
  return id;
}

void AudioManager::DestroyListener(ListenerID id) { m_Listeners.erase(id); }

void AudioManager::SetListenerPosition(ListenerID id, const Vector3 &pos) {
  if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
    it->second.posX = pos.x;
    it->second.posY = pos.y;
    it->second.posZ = pos.z;
  }
}

void AudioManager::SetListenerPosition(ListenerID id, float x, float y,
                                       float z) {
  if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
    it->second.posX = x;
    it->second.posY = y;
    it->second.posZ = z;
  }
}

void AudioManager::SetListenerVelocity(ListenerID id, const Vector3 &vel) {
  if (auto it = m_Listeners.find(id); it != m_Listeners.end()) {
    it->second.velX = vel.x;
    it->second.velY = vel.y;
    it->second.velZ = vel.z;
  }
}

void AudioManager::SetListenerOrientation(ListenerID id, const Vector3 &forward,
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

void AudioManager::CreateBus(const std::string &name) {
  m_Buses[name] = std::make_shared<Bus>(name);
}

Result<std::shared_ptr<Bus>> AudioManager::GetBus(const std::string &name) {
  auto it = m_Buses.find(name);
  if (it == m_Buses.end()) {
    return Error(ErrorCode::BusNotFound, "Bus not found: " + name);
  }
  return it->second;
}

void AudioManager::CreateSnapshot(const std::string &name) {
  m_Snapshots[name] = Snapshot();
}

void AudioManager::SetSnapshotBusVolume(const std::string &snap,
                                        const std::string &bus, float volume) {
  m_Snapshots[snap].SetBusState(bus, BusState{volume});
}

Status AudioManager::ApplySnapshot(const std::string &name, float fadeSeconds) {
  auto it = m_Snapshots.find(name);
  if (it == m_Snapshots.end()) {
    return Error(ErrorCode::SnapshotNotFound, "Snapshot not found: " + name);
  }
  const auto &states = it->second.GetStates();
  for (const auto &[busName, state] : states) {
    if (m_Buses.count(busName))
      m_Buses[busName]->SetTargetVolume(state.volume, fadeSeconds);
  }
  return Ok();
}

void AudioManager::ResetBusVolumes(float fadeSeconds) {
  for (auto &[name, bus] : m_Buses) {
    bus->SetTargetVolume(1.0f, fadeSeconds);
  }
}

void AudioManager::ResetEventVolume(const std::string &eventName,
                                    float fadeSeconds) {
  auto eventResult = m_Bank.FindEvent(eventName);
  if (eventResult) {
    const auto &ed = eventResult.Value();
    const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
    if (m_Buses.count(busName)) {
      m_Buses[busName]->SetTargetVolume(ed.volumeMin, fadeSeconds);
    }
  }
}

void AudioManager::SetMaxVoices(uint32_t maxReal) {
  m_VoicePool.SetMaxVoices(maxReal);
}

uint32_t AudioManager::GetMaxVoices() const {
  return m_VoicePool.GetMaxVoices();
}

void AudioManager::SetStealBehavior(StealBehavior behavior) {
  m_VoicePool.SetStealBehavior(behavior);
}

StealBehavior AudioManager::GetStealBehavior() const {
  return m_VoicePool.GetStealBehavior();
}

uint32_t AudioManager::GetActiveVoiceCount() const {
  return m_VoicePool.GetActiveVoiceCount();
}

uint32_t AudioManager::GetRealVoiceCount() const {
  return m_VoicePool.GetRealVoiceCount();
}

uint32_t AudioManager::GetVirtualVoiceCount() const {
  return m_VoicePool.GetVirtualVoiceCount();
}

void AudioManager::AddMixZone(const std::string &name,
                              const std::string &snapshotName,
                              const Vector3 &pos, float inner, float outer,
                              uint8_t priority, float fadeIn, float fadeOut) {
  m_MixZones.emplace_back(std::make_shared<MixZone>(
      name, snapshotName, pos, inner, outer, priority, fadeIn, fadeOut));
}

void AudioManager::RemoveMixZone(const std::string &name) {
  m_MixZones.erase(
      std::remove_if(m_MixZones.begin(), m_MixZones.end(),
                     [&name](const auto &z) { return z->GetName() == name; }),
      m_MixZones.end());
}

void AudioManager::SetZoneEnterCallback(ZoneEnterCallback cb) {
  m_ZoneEnterCallback = cb;
}

void AudioManager::SetZoneExitCallback(ZoneExitCallback cb) {
  m_ZoneExitCallback = cb;
}

const std::string &AudioManager::GetActiveMixZone() const {
  return m_ActiveMixZone;
}

SoLoud::Soloud &AudioManager::Engine() { return m_Engine; }

Status AudioManager::CreateReverbBus(const std::string &name, float roomSize,
                                     float damp, float wet, float width) {
  if (m_ReverbBuses.count(name) > 0) {
    return Error(ErrorCode::BusAlreadyExists,
                 "Reverb bus already exists: " + name);
  }

  auto reverbBus = std::make_shared<ReverbBus>(name);
  reverbBus->SetParams(wet, roomSize, damp, width);

  if (!reverbBus->Init(m_Engine)) {
    return Error(ErrorCode::ReverbBusInitFailed,
                 "Failed to initialize reverb bus: " + name);
  }

  m_ReverbBuses[name] = reverbBus;
  return Ok();
}

Status AudioManager::CreateReverbBus(const std::string &name,
                                     ReverbPreset preset) {
  if (m_ReverbBuses.count(name) > 0) {
    return Error(ErrorCode::BusAlreadyExists,
                 "Reverb bus already exists: " + name);
  }

  auto reverbBus = std::make_shared<ReverbBus>(name);
  reverbBus->ApplyPreset(preset);

  if (!reverbBus->Init(m_Engine)) {
    return Error(ErrorCode::ReverbBusInitFailed,
                 "Failed to initialize reverb bus: " + name);
  }

  m_ReverbBuses[name] = reverbBus;
  return Ok();
}

Result<std::shared_ptr<ReverbBus>>
AudioManager::GetReverbBus(const std::string &name) {
  auto it = m_ReverbBuses.find(name);
  if (it == m_ReverbBuses.end()) {
    return Error(ErrorCode::ReverbBusNotFound, "Reverb bus not found: " + name);
  }
  return it->second;
}

void AudioManager::SetReverbParams(const std::string &name, float wet,
                                   float roomSize, float damp, float fadeTime) {
  auto busResult = GetReverbBus(name);
  if (busResult) {
    auto &bus = busResult.Value();
    bus->SetWet(wet, fadeTime);
    bus->SetRoomSize(roomSize, fadeTime);
    bus->SetDamp(damp, fadeTime);
  }
}

void AudioManager::AddReverbZone(const std::string &name,
                                 const std::string &reverbBusName,
                                 const Vector3 &pos, float inner, float outer,
                                 uint8_t priority) {
  m_ReverbZones.emplace_back(std::make_shared<ReverbZone>(
      name, reverbBusName, pos, inner, outer, priority));
}

void AudioManager::RemoveReverbZone(const std::string &name) {
  m_ReverbZones.erase(
      std::remove_if(m_ReverbZones.begin(), m_ReverbZones.end(),
                     [&name](const auto &z) { return z->GetName() == name; }),
      m_ReverbZones.end());
}

void AudioManager::SetSnapshotReverbParams(const std::string &snapshotName,
                                           const std::string &reverbBusName,
                                           float wet, float roomSize,
                                           float damp, float width) {
  m_Snapshots[snapshotName].SetReverbState(
      reverbBusName, ReverbBusState{wet, roomSize, damp, width});
}

std::vector<std::string> AudioManager::GetActiveReverbZones() const {
  std::vector<std::string> active;
  for (const auto &zone : m_ReverbZones) {
    if (zone->IsActive()) {
      active.push_back(zone->GetName());
    }
  }
  return active;
}

void AudioManager::SetOcclusionQueryCallback(OcclusionQueryCallback callback) {
  m_OcclusionProcessor.SetQueryCallback(std::move(callback));
}

void AudioManager::RegisterOcclusionMaterial(const OcclusionMaterial &mat) {
  m_OcclusionProcessor.RegisterMaterial(mat);
}

void AudioManager::SetOcclusionEnabled(bool enabled) {
  m_OcclusionProcessor.SetEnabled(enabled);
}

void AudioManager::SetOcclusionThreshold(float threshold) {
  m_OcclusionProcessor.SetOcclusionThreshold(threshold);
}

void AudioManager::SetOcclusionSmoothingTime(float seconds) {
  m_OcclusionProcessor.SetSmoothingTime(seconds);
}

void AudioManager::SetOcclusionUpdateRate(float hz) {
  m_OcclusionProcessor.SetUpdateRate(hz);
}

void AudioManager::SetOcclusionLowPassRange(float minFreq, float maxFreq) {
  m_OcclusionProcessor.SetLowPassRange(minFreq, maxFreq);
}

void AudioManager::SetOcclusionVolumeReduction(float maxReduction) {
  m_OcclusionProcessor.SetVolumeReduction(maxReduction);
}

bool AudioManager::IsOcclusionEnabled() const {
  return m_OcclusionProcessor.IsEnabled();
}

void AudioManager::UpdateMixZones(const Vector3 &listenerPos) {
  // Update all mix zones
  for (auto &zone : m_MixZones) {
    zone->Update(listenerPos);
  }

  // Find highest priority active zone
  MixZone *bestZone = nullptr;
  for (auto &zone : m_MixZones) {
    if (!zone->IsActive())
      continue;
    if (!bestZone || zone->GetPriority() > bestZone->GetPriority() ||
        (zone->GetPriority() == bestZone->GetPriority() &&
         zone->GetBlendFactor() > bestZone->GetBlendFactor())) {
      bestZone = zone.get();
    }
  }

  // Handle zone transitions
  std::string newActiveZone = bestZone ? bestZone->GetName() : "";
  if (newActiveZone != m_ActiveMixZone) {
    // Exit previous zone
    if (!m_ActiveMixZone.empty()) {
      if (m_ZoneExitCallback) {
        m_ZoneExitCallback(m_ActiveMixZone);
      }
      // If exiting to no zone, reset volumes
      if (newActiveZone.empty()) {
        ResetBusVolumes(0.5f);
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
    float blend = bestZone->GetBlendFactor();
    ApplySnapshot(bestZone->GetSnapshotName(),
                  blend * bestZone->GetFadeInTime());
  }
}

void AudioManager::UpdateReverbZones(const Vector3 &listenerPos) {
  // Track total influence per reverb bus
  std::unordered_map<std::string, float> busInfluence;

  // Update all reverb zones and accumulate influence
  for (auto &zone : m_ReverbZones) {
    float influence = zone->Update(listenerPos);
    if (influence > 0.0f) {
      const std::string &busName = zone->GetReverbBusName();
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
    bus->SetWet(targetWet, 0.1f);       // Small fade time for smoothness
  }
}

} // namespace Orpheus
