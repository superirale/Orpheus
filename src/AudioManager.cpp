#include "../include/AudioManager.h"
#include "../include/AudioZone.h"
#include "../include/Bus.h"
#include "../include/Ducker.h"
#include "../include/Event.h"
#include "../include/Listener.h"
#include "../include/Log.h"
#include "../include/MixZone.h"
#include "../include/OcclusionProcessor.h"
#include "../include/Parameter.h"
#include "../include/ReverbZone.h"
#include "../include/Snapshot.h"
#include "../include/VoicePool.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace Orpheus {

// =============================================================================
// AudioManager::Impl - Private Implementation
// =============================================================================

class AudioManager::Impl {
public:
  SoLoud::Soloud engine;
  SoundBank bank;
  AudioEvent event;
  VoicePool voicePool;
  std::unordered_map<std::string, std::shared_ptr<Bus>> buses;
  std::vector<std::shared_ptr<AudioZone>> zones;
  std::unordered_map<ListenerID, Listener> listeners;
  ListenerID nextListenerID = 1;
  std::unordered_map<std::string, Parameter> parameters;
  std::mutex paramMutex;

  std::unordered_map<std::string, Snapshot> snapshots;

  std::vector<std::shared_ptr<MixZone>> mixZones;
  std::string activeMixZone;
  ZoneEnterCallback zoneEnterCallback;
  ZoneExitCallback zoneExitCallback;

  std::unordered_map<std::string, std::shared_ptr<ReverbBus>> reverbBuses;
  std::vector<std::shared_ptr<ReverbZone>> reverbZones;

  OcclusionProcessor occlusionProcessor;

  Ducker ducker;

  Impl() : event(engine, bank) {}
};

// =============================================================================
// AudioManager - Public API Implementation
// =============================================================================

AudioManager::AudioManager() : pImpl(std::make_unique<Impl>()) {}

AudioManager::~AudioManager() { Shutdown(); }

AudioManager::AudioManager(AudioManager &&) noexcept = default;
AudioManager &AudioManager::operator=(AudioManager &&) noexcept = default;

Status AudioManager::Init() {
  ORPHEUS_DEBUG("Initializing SoLoud engine");
  SoLoud::result r = pImpl->engine.init();
  if (r != SoLoud::SO_NO_ERROR) {
    ORPHEUS_ERROR("SoLoud init failed with code: " << r);
    return Error(ErrorCode::EngineInitFailed,
                 "SoLoud init failed with code: " + std::to_string(r));
  }
  // Create default buses
  CreateBus("Master");
  CreateBus("SFX");
  CreateBus("Music");

  // Set up bus router for AudioEvent
  pImpl->event.SetBusRouter([this](AudioHandle h, const std::string &busName) {
    if (pImpl->buses.count(busName)) {
      pImpl->buses[busName]->AddHandle(pImpl->engine, h);
    }
  });

  ORPHEUS_INFO("Orpheus audio engine initialized");
  return Ok();
}

void AudioManager::Shutdown() {
  if (pImpl) {
    pImpl->engine.deinit();
  }
}

void AudioManager::Update(float dt) {
  for (auto &[_, bus] : pImpl->buses)
    bus->Update(dt);

  // Get first listener position for voice pool and zones
  Vector3 listenerPos{0, 0, 0};
  for (auto &[id, listener] : pImpl->listeners) {
    if (!listener.active)
      continue;
    listenerPos = {listener.posX, listener.posY, listener.posZ};
    pImpl->engine.set3dListenerParameters(
        listener.posX, listener.posY, listener.posZ, listener.velX,
        listener.velY, listener.velZ, listener.forwardX, listener.forwardY,
        listener.forwardZ, listener.upX, listener.upY, listener.upZ);
    for (auto &zone : pImpl->zones) {
      zone->Update(listenerPos);
    }
  }

  // Update voice pool (virtualization/promotion)
  pImpl->voicePool.Update(dt, listenerPos);

  // Process voice state changes
  for (size_t i = 0; i < pImpl->voicePool.GetVoiceCount(); ++i) {
    Voice *voice = pImpl->voicePool.GetVoiceAt(i);
    if (!voice || voice->IsStopped())
      continue;

    // Handle voices that need to start playing
    if (voice->IsReal() && voice->handle == 0) {
      voice->handle = pImpl->event.Play(voice->eventName);
    }
    // Handle voices that became virtual
    else if (voice->IsVirtual() && voice->handle != 0) {
      pImpl->engine.stop(voice->handle);
      voice->handle = 0;
    }

    // Update occlusion for real voices
    if (voice->IsReal() && voice->handle != 0) {
      pImpl->occlusionProcessor.Update(*voice, listenerPos, dt);
      pImpl->occlusionProcessor.ApplyDSP(pImpl->engine, *voice);
    }
  }

  // Update mix zones and apply highest priority active snapshot
  UpdateMixZones(listenerPos);

  // Update reverb zones (calculate zone influence on reverb buses)
  UpdateReverbZones(listenerPos);

  // Update ducking (sidechaining)
  pImpl->ducker.Update(dt, pImpl->buses, [this](const std::string &busName) {
    // Check if any voices are active on this bus
    // For now, check if bus has any active handles
    auto it = pImpl->buses.find(busName);
    if (it == pImpl->buses.end()) {
      return false;
    }
    // Check if engine has any active voices for this bus
    // This is a simplified check - in practice you'd track voice count per bus
    return pImpl->engine.getActiveVoiceCount() > 0;
  });

  pImpl->engine.update3dAudio();
}

Result<VoiceID> AudioManager::PlayEvent(const std::string &name,
                                        Vector3 position) {
  auto eventResult = pImpl->bank.FindEvent(name);
  if (eventResult.IsError()) {
    return eventResult.GetError();
  }
  const auto &ed = eventResult.Value();

  // Create distance settings from event descriptor
  DistanceSettings distSettings;
  distSettings.maxDistance = ed.maxDistance;

  // Allocate voice in pool
  Voice *voice =
      pImpl->voicePool.AllocateVoice(name, ed.priority, position, distSettings);
  if (!voice) {
    return Error(ErrorCode::VoiceAllocationFailed, "Failed to allocate voice");
  }

  voice->volume = ed.volumeMin;

  // Try to make it real (may steal another voice)
  if (pImpl->voicePool.MakeReal(voice)) {
    voice->handle = pImpl->event.Play(name);
  }

  return voice->id;
}

Result<AudioHandle> AudioManager::PlayEventDirect(const std::string &name) {
  AudioHandle h = pImpl->event.Play(name);
  if (h == 0) {
    return Error(ErrorCode::PlaybackFailed, "Failed to play event: " + name);
  }
  return h;
}

void AudioManager::RegisterEvent(const EventDescriptor &ed) {
  pImpl->bank.RegisterEvent(ed);
}

Status AudioManager::RegisterEvent(const std::string &jsonString) {
  return pImpl->bank.RegisterEventFromJson(jsonString);
}

Status AudioManager::LoadEventsFromFile(const std::string &jsonPath) {
  return pImpl->bank.LoadFromJsonFile(jsonPath);
}

void AudioManager::SetGlobalParameter(const std::string &name, float value) {
  std::lock_guard<std::mutex> lock(pImpl->paramMutex);
  pImpl->parameters[name].Set(value);
}

Parameter *AudioManager::GetParam(const std::string &name) {
  std::lock_guard<std::mutex> lock(pImpl->paramMutex);
  return &pImpl->parameters[name];
}

void AudioManager::AddAudioZone(const std::string &eventName,
                                const Vector3 &pos, float inner, float outer) {
  pImpl->zones.emplace_back(std::make_shared<AudioZone>(
      eventName, pos, inner, outer,
      [this](const std::string &name) {
        auto result = this->PlayEventDirect(name);
        return result.ValueOr(0);
      },
      [this](AudioHandle h, float v) { pImpl->engine.setVolume(h, v); },
      [this](AudioHandle h) { pImpl->engine.stop(h); },
      [this](AudioHandle h) { return pImpl->engine.isValidVoiceHandle(h); }));
}

void AudioManager::AddAudioZone(const std::string &eventName,
                                const Vector3 &pos, float inner, float outer,
                                const std::string &snapshotName, float fadeIn,
                                float fadeOut) {
  pImpl->zones.emplace_back(std::make_shared<AudioZone>(
      eventName, pos, inner, outer,
      [this](const std::string &name) {
        auto result = this->PlayEventDirect(name);
        return result.ValueOr(0);
      },
      [this](AudioHandle h, float v) { pImpl->engine.setVolume(h, v); },
      [this](AudioHandle h) { pImpl->engine.stop(h); },
      [this](AudioHandle h) { return pImpl->engine.isValidVoiceHandle(h); },
      snapshotName,
      [this](const std::string &snap, float fade) {
        this->ApplySnapshot(snap, fade);
      },
      [this](float fade) { this->ResetBusVolumes(fade); }, fadeIn, fadeOut));
}

ListenerID AudioManager::CreateListener() {
  ListenerID id = pImpl->nextListenerID++;
  pImpl->listeners[id] = Listener{id};
  return id;
}

void AudioManager::DestroyListener(ListenerID id) {
  pImpl->listeners.erase(id);
}

void AudioManager::SetListenerPosition(ListenerID id, const Vector3 &pos) {
  if (auto it = pImpl->listeners.find(id); it != pImpl->listeners.end()) {
    it->second.posX = pos.x;
    it->second.posY = pos.y;
    it->second.posZ = pos.z;
  }
}

void AudioManager::SetListenerPosition(ListenerID id, float x, float y,
                                       float z) {
  if (auto it = pImpl->listeners.find(id); it != pImpl->listeners.end()) {
    it->second.posX = x;
    it->second.posY = y;
    it->second.posZ = z;
  }
}

void AudioManager::SetListenerVelocity(ListenerID id, const Vector3 &vel) {
  if (auto it = pImpl->listeners.find(id); it != pImpl->listeners.end()) {
    it->second.velX = vel.x;
    it->second.velY = vel.y;
    it->second.velZ = vel.z;
  }
}

void AudioManager::SetListenerOrientation(ListenerID id, const Vector3 &forward,
                                          const Vector3 &up) {
  if (auto it = pImpl->listeners.find(id); it != pImpl->listeners.end()) {
    it->second.forwardX = forward.x;
    it->second.forwardY = forward.y;
    it->second.forwardZ = forward.z;
    it->second.upX = up.x;
    it->second.upY = up.y;
    it->second.upZ = up.z;
  }
}

void AudioManager::CreateBus(const std::string &name) {
  pImpl->buses[name] = std::make_shared<Bus>(name);
}

Result<std::shared_ptr<Bus>> AudioManager::GetBus(const std::string &name) {
  auto it = pImpl->buses.find(name);
  if (it == pImpl->buses.end()) {
    return Error(ErrorCode::BusNotFound, "Bus not found: " + name);
  }
  return it->second;
}

void AudioManager::CreateSnapshot(const std::string &name) {
  pImpl->snapshots[name] = Snapshot();
}

void AudioManager::SetSnapshotBusVolume(const std::string &snap,
                                        const std::string &bus, float volume) {
  pImpl->snapshots[snap].SetBusState(bus, BusState{volume});
}

Status AudioManager::ApplySnapshot(const std::string &name, float fadeSeconds) {
  auto it = pImpl->snapshots.find(name);
  if (it == pImpl->snapshots.end()) {
    return Error(ErrorCode::SnapshotNotFound, "Snapshot not found: " + name);
  }
  const auto &states = it->second.GetStates();
  for (const auto &[busName, state] : states) {
    if (pImpl->buses.count(busName))
      pImpl->buses[busName]->SetTargetVolume(state.volume, fadeSeconds);
  }
  return Ok();
}

void AudioManager::ResetBusVolumes(float fadeSeconds) {
  for (auto &[name, bus] : pImpl->buses) {
    bus->SetTargetVolume(1.0f, fadeSeconds);
  }
}

void AudioManager::ResetEventVolume(const std::string &eventName,
                                    float fadeSeconds) {
  auto eventResult = pImpl->bank.FindEvent(eventName);
  if (eventResult) {
    const auto &ed = eventResult.Value();
    const std::string &busName = ed.bus.empty() ? "Master" : ed.bus;
    if (pImpl->buses.count(busName)) {
      pImpl->buses[busName]->SetTargetVolume(ed.volumeMin, fadeSeconds);
    }
  }
}

void AudioManager::SetMaxVoices(uint32_t maxReal) {
  pImpl->voicePool.SetMaxVoices(maxReal);
}

uint32_t AudioManager::GetMaxVoices() const {
  return pImpl->voicePool.GetMaxVoices();
}

void AudioManager::SetStealBehavior(StealBehavior behavior) {
  pImpl->voicePool.SetStealBehavior(behavior);
}

StealBehavior AudioManager::GetStealBehavior() const {
  return pImpl->voicePool.GetStealBehavior();
}

uint32_t AudioManager::GetActiveVoiceCount() const {
  return pImpl->voicePool.GetActiveVoiceCount();
}

uint32_t AudioManager::GetRealVoiceCount() const {
  return pImpl->voicePool.GetRealVoiceCount();
}

uint32_t AudioManager::GetVirtualVoiceCount() const {
  return pImpl->voicePool.GetVirtualVoiceCount();
}

void AudioManager::AddMixZone(const std::string &name,
                              const std::string &snapshotName,
                              const Vector3 &pos, float inner, float outer,
                              uint8_t priority, float fadeIn, float fadeOut) {
  pImpl->mixZones.emplace_back(std::make_shared<MixZone>(
      name, snapshotName, pos, inner, outer, priority, fadeIn, fadeOut));
}

void AudioManager::RemoveMixZone(const std::string &name) {
  pImpl->mixZones.erase(
      std::remove_if(pImpl->mixZones.begin(), pImpl->mixZones.end(),
                     [&name](const auto &z) { return z->GetName() == name; }),
      pImpl->mixZones.end());
}

void AudioManager::SetZoneEnterCallback(ZoneEnterCallback cb) {
  pImpl->zoneEnterCallback = cb;
}

void AudioManager::SetZoneExitCallback(ZoneExitCallback cb) {
  pImpl->zoneExitCallback = cb;
}

const std::string &AudioManager::GetActiveMixZone() const {
  return pImpl->activeMixZone;
}

SoLoud::Soloud &AudioManager::Engine() { return pImpl->engine; }

Status AudioManager::CreateReverbBus(const std::string &name, float roomSize,
                                     float damp, float wet, float width) {
  if (pImpl->reverbBuses.count(name) > 0) {
    return Error(ErrorCode::BusAlreadyExists,
                 "Reverb bus already exists: " + name);
  }

  auto reverbBus = std::make_shared<ReverbBus>(name);
  reverbBus->SetParams(wet, roomSize, damp, width);

  if (!reverbBus->Init(pImpl->engine)) {
    return Error(ErrorCode::ReverbBusInitFailed,
                 "Failed to initialize reverb bus: " + name);
  }

  pImpl->reverbBuses[name] = reverbBus;
  return Ok();
}

Status AudioManager::CreateReverbBus(const std::string &name,
                                     ReverbPreset preset) {
  if (pImpl->reverbBuses.count(name) > 0) {
    return Error(ErrorCode::BusAlreadyExists,
                 "Reverb bus already exists: " + name);
  }

  auto reverbBus = std::make_shared<ReverbBus>(name);
  reverbBus->ApplyPreset(preset);

  if (!reverbBus->Init(pImpl->engine)) {
    return Error(ErrorCode::ReverbBusInitFailed,
                 "Failed to initialize reverb bus: " + name);
  }

  pImpl->reverbBuses[name] = reverbBus;
  return Ok();
}

Result<std::shared_ptr<ReverbBus>>
AudioManager::GetReverbBus(const std::string &name) {
  auto it = pImpl->reverbBuses.find(name);
  if (it == pImpl->reverbBuses.end()) {
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
  pImpl->reverbZones.emplace_back(std::make_shared<ReverbZone>(
      name, reverbBusName, pos, inner, outer, priority));
}

void AudioManager::RemoveReverbZone(const std::string &name) {
  pImpl->reverbZones.erase(
      std::remove_if(pImpl->reverbZones.begin(), pImpl->reverbZones.end(),
                     [&name](const auto &z) { return z->GetName() == name; }),
      pImpl->reverbZones.end());
}

void AudioManager::SetSnapshotReverbParams(const std::string &snapshotName,
                                           const std::string &reverbBusName,
                                           float wet, float roomSize,
                                           float damp, float width) {
  pImpl->snapshots[snapshotName].SetReverbState(
      reverbBusName, ReverbBusState{wet, roomSize, damp, width});
}

std::vector<std::string> AudioManager::GetActiveReverbZones() const {
  std::vector<std::string> active;
  for (const auto &zone : pImpl->reverbZones) {
    if (zone->IsActive()) {
      active.push_back(zone->GetName());
    }
  }
  return active;
}

void AudioManager::SetOcclusionQueryCallback(OcclusionQueryCallback callback) {
  pImpl->occlusionProcessor.SetQueryCallback(std::move(callback));
}

void AudioManager::RegisterOcclusionMaterial(const OcclusionMaterial &mat) {
  pImpl->occlusionProcessor.RegisterMaterial(mat);
}

void AudioManager::SetOcclusionEnabled(bool enabled) {
  pImpl->occlusionProcessor.SetEnabled(enabled);
}

void AudioManager::SetOcclusionThreshold(float threshold) {
  pImpl->occlusionProcessor.SetOcclusionThreshold(threshold);
}

void AudioManager::SetOcclusionSmoothingTime(float seconds) {
  pImpl->occlusionProcessor.SetSmoothingTime(seconds);
}

void AudioManager::SetOcclusionUpdateRate(float hz) {
  pImpl->occlusionProcessor.SetUpdateRate(hz);
}

void AudioManager::SetOcclusionLowPassRange(float minFreq, float maxFreq) {
  pImpl->occlusionProcessor.SetLowPassRange(minFreq, maxFreq);
}

void AudioManager::SetOcclusionVolumeReduction(float maxReduction) {
  pImpl->occlusionProcessor.SetVolumeReduction(maxReduction);
}

bool AudioManager::IsOcclusionEnabled() const {
  return pImpl->occlusionProcessor.IsEnabled();
}

void AudioManager::AddDuckingRule(const std::string &targetBus,
                                  const std::string &sidechainBus,
                                  float duckLevel, float attackTime,
                                  float releaseTime, float holdTime) {
  DuckingRule rule;
  rule.targetBus = targetBus;
  rule.sidechainBus = sidechainBus;
  rule.duckLevel = duckLevel;
  rule.attackTime = attackTime;
  rule.releaseTime = releaseTime;
  rule.holdTime = holdTime;
  pImpl->ducker.AddRule(rule);
}

void AudioManager::RemoveDuckingRule(const std::string &targetBus,
                                     const std::string &sidechainBus) {
  pImpl->ducker.RemoveRule(targetBus, sidechainBus);
}

bool AudioManager::IsDucking(const std::string &targetBus) const {
  return pImpl->ducker.IsDucking(targetBus);
}

void AudioManager::UpdateMixZones(const Vector3 &listenerPos) {
  // Update all mix zones
  for (auto &zone : pImpl->mixZones) {
    zone->Update(listenerPos);
  }

  // Find highest priority active zone
  MixZone *bestZone = nullptr;
  for (auto &zone : pImpl->mixZones) {
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
  if (newActiveZone != pImpl->activeMixZone) {
    // Exit previous zone
    if (!pImpl->activeMixZone.empty()) {
      if (pImpl->zoneExitCallback) {
        pImpl->zoneExitCallback(pImpl->activeMixZone);
      }
      // If exiting to no zone, reset volumes
      if (newActiveZone.empty()) {
        ResetBusVolumes(0.5f);
      }
    }
    // Enter new zone
    if (!newActiveZone.empty() && pImpl->zoneEnterCallback) {
      pImpl->zoneEnterCallback(newActiveZone);
    }
    pImpl->activeMixZone = newActiveZone;
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
  for (auto &zone : pImpl->reverbZones) {
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
  for (auto &[busName, bus] : pImpl->reverbBuses) {
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
