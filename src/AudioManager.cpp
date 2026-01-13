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
#include "HDRFilter_Internal.h"

#include <algorithm>
#include <mutex>
#include <random>
#include <unordered_map>

namespace Orpheus {

// Thread-local random engine for randomization
static thread_local std::mt19937 s_RandomEngine{std::random_device{}()};

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

  // Doppler settings
  bool dopplerEnabled = true;
  float speedOfSound = 343.0f; // m/s at 20°C
  float dopplerFactor = 1.0f;  // Exaggeration factor

  // Music manager
  std::unique_ptr<MusicManager> musicManager;

  // RTPC bindings
  std::vector<RTPCBinding> rtpcBindings;

  // Zone crossfading
  bool zoneCrossfadeEnabled = true;

  // Convolution reverbs
  std::unordered_map<std::string, std::unique_ptr<ConvolutionReverb>>
      convolutionReverbs;

  // HDR Audio
  HDRMixer hdrMixer;
  std::unique_ptr<HDRFilter> hdrFilter;

  // Ray-traced Acoustics
  AcousticRayTracer rayTracer;

  NativeEngineHandle GetEngineHandle() { return NativeEngineHandle{&engine}; }

  Impl() : event(NativeEngineHandle{nullptr}, bank) {
    // Note: event is re-initialized in Init() with valid engine handle
  }

  void InitSubsystems() {
    NativeEngineHandle engineHandle{&engine};
    event = AudioEvent(engineHandle, bank);
    musicManager = std::make_unique<MusicManager>(engineHandle, bank);
    hdrFilter = std::make_unique<HDRFilter>(&hdrMixer);
  }
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

  // Initialize subsystems with valid engine handle
  pImpl->InitSubsystems();

  // Create default buses
  CreateBus("Master");
  CreateBus("SFX");
  CreateBus("Music");

  // Set up bus router for AudioEvent
  pImpl->event.SetBusRouter([this](AudioHandle h, const std::string &busName) {
    if (pImpl->buses.count(busName)) {
      pImpl->buses[busName]->AddHandle(pImpl->GetEngineHandle(), h);
    }
  });

  // Attach HDR filter to engine for loudness metering
  pImpl->engine.setGlobalFilter(0, pImpl->hdrFilter.get());

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
    // Update zones with optional crossfading
    if (pImpl->zoneCrossfadeEnabled) {
      // Collect volumes for all zones
      std::vector<std::pair<AudioZone *, float>> activeZones;
      float totalVolume = 0.0f;

      for (auto &zone : pImpl->zones) {
        float vol = zone->GetComputedVolume(listenerPos);
        if (vol > 0.0f) {
          activeZones.push_back({zone.get(), vol});
          totalVolume += vol;
        }
      }

      // Normalize if total exceeds 1.0
      float normalizer = (totalVolume > 1.0f) ? 1.0f / totalVolume : 1.0f;

      // Apply normalized volumes
      for (auto &[zone, vol] : activeZones) {
        zone->EnsurePlaying();
        zone->ApplyVolume(vol * normalizer);
      }

      // Stop zones that are no longer active
      for (auto &zone : pImpl->zones) {
        float vol = zone->GetComputedVolume(listenerPos);
        if (vol <= 0.0f) {
          zone->StopPlaying();
        }
      }
    } else {
      // Original behavior: independent zone volumes
      for (auto &zone : pImpl->zones) {
        zone->Update(listenerPos);
      }
    }
  }

  // Update voice pool (virtualization/promotion)
  pImpl->voicePool.Update(dt, listenerPos);

  // Process voice state changes
  for (size_t i = 0; i < pImpl->voicePool.GetVoiceCount(); ++i) {
    Voice *voice = pImpl->voicePool.GetVoiceAt(i);
    if (!voice || voice->IsStopped())
      continue;

    // Handle delay timer
    if (voice->isWaitingForDelay) {
      voice->delayTimer -= dt;
      if (voice->delayTimer <= 0.0f) {
        voice->isWaitingForDelay = false;
      } else {
        continue; // Still waiting
      }
    }

    // Handle voices that need to start playing
    if (voice->IsReal() && voice->handle == 0) {
      // Logic to pick sound was done in PlayEvent, but if we looped/next-item,
      // we need to pick it here if we just cleared the handle.

      // Determine what to play
      std::string soundToPlay = voice->eventName; // Default

      // If playlist is active, pick safe sound
      if (!voice->playlist.empty()) {
        if (voice->playlistIndex < voice->playlist.size()) {
          soundToPlay = voice->playlist[voice->playlistIndex];
        }
      }

      // Find the event descriptor again to pass settings
      auto eventResult =
          pImpl->bank.FindEvent(voice->eventName); // Original event name
      if (eventResult) {
        const auto &ed = eventResult.Value();
        if (!voice->playlist.empty()) {
          voice->handle = pImpl->event.PlayFromEvent(soundToPlay, ed);
        } else {
          voice->handle = pImpl->event.Play(voice->eventName);
        }
      } else {
        // Fallback if event somehow gone, shouldn't happen
        voice->handle = pImpl->event.Play(voice->eventName);
      }
    }
    // Handle voices that became virtual
    else if (voice->IsVirtual() && voice->handle != 0) {
      pImpl->engine.stop(voice->handle);
      voice->handle = 0;
    }
    // Handle finished voices (Real, handle was valid, now invalid)
    else if (voice->IsReal() && voice->handle != 0 &&
             !pImpl->engine.isValidVoiceHandle(voice->handle)) {

      // Voice finished playing. Check playlist logic.
      bool shouldPlayNext = false;

      if (!voice->playlist.empty()) {
        if (voice->playlistMode == PlaylistMode::Sequential ||
            voice->playlistMode == PlaylistMode::Shuffle) {

          voice->playlistIndex++;
          if (voice->playlistIndex >=
              static_cast<int>(voice->playlist.size())) {
            // Loop or Stop
            if (voice->loopPlaylist) {
              voice->playlistIndex = 0;
              shouldPlayNext = true;
            }
          } else {
            shouldPlayNext = true;
          }
        } else if (voice->playlistMode == PlaylistMode::Random) {
          // Random mode usually plays once.
          // But if loopPlaylist is true, we play another random sound.
          if (voice->loopPlaylist) {
            std::uniform_int_distribution<size_t> dist(
                0, voice->playlist.size() - 1);
            voice->playlistIndex = dist(s_RandomEngine);
            shouldPlayNext = true;
          }
        }
      } else {
        // Single Event
        // Assuming SoundBank handles basic looping via SoLoud?
        // If not, and we want to "loop" a single event via this system:
        if (voice->loopPlaylist) {
          shouldPlayNext = true;
        }
      }

      if (shouldPlayNext) {
        voice->handle = 0; // Reset handle
        if (voice->interval > 0.0f) {
          voice->delayTimer = voice->interval;
          voice->isWaitingForDelay = true;
        }
        // If no interval, it will be picked up in next frame (or we could loop
        // goto, but simpler to wait frame) Actually, if we don't set
        // isWaitingForDelay, we fall through? No, because we are in an `else
        // if`. Next frame `voice->handle == 0` will trigger play.
      } else {
        // Really finished.
        voice->handle = 0;
        voice->state = VoiceState::Stopped; // Mark as free/stopped
      }
    }

    // Update occlusion for real voices
    if (voice->IsReal() && voice->handle != 0) {
      pImpl->occlusionProcessor.Update(*voice, listenerPos, dt);
      pImpl->occlusionProcessor.ApplyDSP(pImpl->GetEngineHandle(), *voice);

      // Calculate and apply Doppler effect
      if (pImpl->dopplerEnabled) {
        // Get listener velocity
        Vector3 listenerVel{0, 0, 0};
        for (auto &[lid, l] : pImpl->listeners) {
          if (l.active) {
            listenerVel = {l.velX, l.velY, l.velZ};
            break;
          }
        }

        // Calculate direction from listener to source
        float dx = voice->position.x - listenerPos.x;
        float dy = voice->position.y - listenerPos.y;
        float dz = voice->position.z - listenerPos.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist > 0.001f) {
          // Normalize direction
          float dirX = dx / dist;
          float dirY = dy / dist;
          float dirZ = dz / dist;

          // Calculate relative velocities along the direction
          float sourceVelTowards = voice->velocity.x * dirX +
                                   voice->velocity.y * dirY +
                                   voice->velocity.z * dirZ;
          float listenerVelTowards = listenerVel.x * dirX +
                                     listenerVel.y * dirY +
                                     listenerVel.z * dirZ;

          // Relative velocity (positive = receding, negative = approaching)
          float relativeVel =
              (sourceVelTowards - listenerVelTowards) * pImpl->dopplerFactor;

          // Doppler formula: pitch = speedOfSound / (speedOfSound + relVel)
          float pitch =
              pImpl->speedOfSound / (pImpl->speedOfSound + relativeVel);

          // Clamp to reasonable range
          pitch = std::max(0.5f, std::min(2.0f, pitch));
          voice->dopplerPitch = pitch;

          // Apply pitch via SoLoud
          pImpl->engine.setRelativePlaySpeed(voice->handle, pitch);
        }
      }

      // Process markers
      double streamTime = pImpl->engine.getStreamTime(voice->handle);
      for (auto &marker : voice->markers) {
        if (!marker.triggered && streamTime >= marker.time) {
          marker.triggered = true;
          if (marker.callback) {
            marker.callback();
          }
        }
      }
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

  // Update interactive music
  pImpl->musicManager->Update(dt);

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
  voice->interval = ed.interval;
  voice->loopPlaylist = ed.loopPlaylist;
  voice->playlist = ed.sounds;
  voice->playlistMode = ed.playlistMode;

  if (!ed.sounds.empty()) {
    // If playlist mode, ensure we have the list
    // Shuffle if needed
    if (ed.playlistMode == PlaylistMode::Shuffle) {
      std::shuffle(voice->playlist.begin(), voice->playlist.end(),
                   s_RandomEngine);
    }
  }

  // Handle Initial Delay
  if (ed.startDelay > 0.0f) {
    voice->isWaitingForDelay = true;
    voice->delayTimer = ed.startDelay;
    // Do NOT play yet. The Update loop will handle it.
  } else {
    // Immediate playback
    // Decide which sound to play
    std::string soundToPlay = name;

    if (!voice->playlist.empty()) {
      if (voice->playlistMode == PlaylistMode::Random) {
        std::uniform_int_distribution<size_t> dist(0,
                                                   voice->playlist.size() - 1);
        voice->playlistIndex = dist(s_RandomEngine);
        soundToPlay = voice->playlist[voice->playlistIndex];
      } else {
        // Sequential / Shuffle (already shuffled)
        voice->playlistIndex = 0;
        soundToPlay = voice->playlist[0];
      }
    }

    // Update event name to actual sound
    // file/event for Play() if needed?
    // Actually Play expects an Event Name, not a file path.
    // Wait, the EventDescriptor has a 'path' but also 'sounds'.
    // If 'sounds' is used, are they file paths or other Event names?
    // Looking at SoundBank.h: "std::vector<std::string> sounds; ///< Multiple
    // sound paths for playlists" So they are paths! But pImpl->event.Play()
    // takes an eventName and looks it up in SoundBank. This implies we need a
    // way to play a raw file or we need temporary event descriptors for
    // playlist items.

    // Correction: AudioEvent::Play logic takes an eventName logic to look up
    // the struct. If we want to support playlists of *files* inside a single
    // Event, we need AudioEvent to support playing a file directly OR
    // AudioEvent::Play should take a path if we change it.

    // Let's look at AudioEvent::Play again.
    // It calls m_Impl->bank->FindEvent(eventName).

    // Problem: The current system relies on looking up the EventDescriptor by
    // name. Use Case: Event "Footsteps" has sounds ["step1.wav", "step2.wav"].
    // When we play "Footsteps", we want to pick one wav.

    // If we change voice->eventName to "step1.wav", FindEvent("step1.wav") will
    // fail.

    // Solution: We need a way to tell AudioEvent "Play this specific file using
    // the settings of THIS event descriptor". Or we modify AudioEvent::Play to
    // optionally take an override path.

    // Let's modify AudioEvent::Play or add a new InternalPlay.
    // Or, since we are inside AudioManager, maybe we can access the engine
    // directly? pImpl->event is a wrapper.

    // Actually, looking at AudioEvent.cpp, it does:
    // auto eventResult = m_Impl->bank->FindEvent(eventName);
    // ...
    // wav->load(ed.path.c_str());

    // If we simply put the chosen sound path into the Voice struct, we can pass
    // it to a modified Play function.

    // Let's assume for now we will refactor Play to support this, or add a
    // helper. For this step, I will stick to the plan of setting up the Voice
    // state. However, I need to resolve how to play a specific file from the
    // playlist.

    // Refined Plan for Play:
    // If using playlist, we need to bypass the standard "FindEvent" lookup in
    // AudioEvent OR we need to update how we call it.

    // NOTE: For this specific tool call, I will write the logic to set up the
    // Voice. I will use a placeholder comment for the actual playing of the
    // playlist item and then address the `AudioEvent` API limitation in the
    // next steps.

    if (pImpl->voicePool.MakeReal(voice)) {
      // If playlist, we need to play the specific file.
      // If not playlist, play the event name.
      if (!voice->playlist.empty()) {
        // We need a new way to play a file with the context of the event
        // pImpl->event.PlaySoundFile(soundToPlay, ed); // Hypothetical
        voice->handle = pImpl->event.PlayFromEvent(soundToPlay, ed);
      } else {
        voice->handle = pImpl->event.Play(name);
      }
    }
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

void AudioManager::AddBoxZone(const std::string &eventName, const Vector3 &min,
                              const Vector3 &max, float fadeDistance) {
  // Create a sphere zone at the center with appropriate radii
  // The center is the midpoint of the box
  Vector3 center = {(min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f,
                    (min.z + max.z) / 2.0f};

  // Calculate half-diagonal as the outer radius
  float dx = (max.x - min.x) / 2.0f;
  float dy = (max.y - min.y) / 2.0f;
  float dz = (max.z - min.z) / 2.0f;
  float innerRadius = std::sqrt(dx * dx + dy * dy + dz * dz);
  float outerRadius = innerRadius + fadeDistance;

  // For now, use sphere approximation
  // TODO: Implement proper box zone with BoxShape
  AddAudioZone(eventName, center, innerRadius, outerRadius);
}

void AudioManager::AddPolygonZone(const std::string &eventName,
                                  const std::vector<Vector2> &points,
                                  float minY, float maxY, float fadeDistance) {
  // Calculate centroid of polygon
  float cx = 0.0f, cz = 0.0f;
  for (const auto &p : points) {
    cx += p.x;
    cz += p.y; // Note: Vector2.y is used for z-axis in 2D polygon
  }
  if (!points.empty()) {
    cx /= static_cast<float>(points.size());
    cz /= static_cast<float>(points.size());
  }

  // Calculate maximum distance from centroid to any vertex
  float maxDist = 0.0f;
  for (const auto &p : points) {
    float dx = p.x - cx;
    float dz = p.y - cz;
    float dist = std::sqrt(dx * dx + dz * dz);
    maxDist = (std::max)(maxDist, dist);
  }

  Vector3 center = {cx, (minY + maxY) / 2.0f, cz};
  float innerRadius = maxDist;
  float outerRadius = maxDist + fadeDistance;

  // For now, use sphere approximation
  // TODO: Implement proper polygon zone with PolygonShape
  AddAudioZone(eventName, center, innerRadius, outerRadius);
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

NativeEngineHandle AudioManager::GetNativeEngine() {
  return pImpl->GetEngineHandle();
}

Status AudioManager::CreateReverbBus(const std::string &name, float roomSize,
                                     float damp, float wet, float width) {
  if (pImpl->reverbBuses.count(name) > 0) {
    return Error(ErrorCode::BusAlreadyExists,
                 "Reverb bus already exists: " + name);
  }

  auto reverbBus = std::make_shared<ReverbBus>(name);
  reverbBus->SetParams(wet, roomSize, damp, width);

  if (!reverbBus->Init(pImpl->GetEngineHandle())) {
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

  if (!reverbBus->Init(pImpl->GetEngineHandle())) {
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

// =============================================================================
// Doppler API
// =============================================================================

void AudioManager::SetVoiceVelocity(VoiceID id, const Vector3 &velocity) {
  for (size_t i = 0; i < pImpl->voicePool.GetVoiceCount(); ++i) {
    Voice *voice = pImpl->voicePool.GetVoiceAt(i);
    if (voice && voice->id == id) {
      voice->velocity = velocity;
      return;
    }
  }
}

void AudioManager::SetDopplerEnabled(bool enabled) {
  pImpl->dopplerEnabled = enabled;
}

void AudioManager::SetSpeedOfSound(float speed) {
  pImpl->speedOfSound = std::max(1.0f, speed);
}

void AudioManager::SetDopplerFactor(float factor) {
  pImpl->dopplerFactor = std::max(0.0f, factor);
}

// =============================================================================
// Markers/Cues API
// =============================================================================

void AudioManager::AddMarker(VoiceID id, float time, const std::string &name,
                             std::function<void()> callback) {
  for (size_t i = 0; i < pImpl->voicePool.GetVoiceCount(); ++i) {
    Voice *voice = pImpl->voicePool.GetVoiceAt(i);
    if (voice && voice->id == id) {
      Marker marker;
      marker.time = time;
      marker.name = name;
      marker.callback = std::move(callback);
      voice->markers.push_back(marker);
      return;
    }
  }
}

void AudioManager::RemoveMarker(VoiceID id, const std::string &name) {
  for (size_t i = 0; i < pImpl->voicePool.GetVoiceCount(); ++i) {
    Voice *voice = pImpl->voicePool.GetVoiceAt(i);
    if (voice && voice->id == id) {
      voice->markers.erase(
          std::remove_if(voice->markers.begin(), voice->markers.end(),
                         [&name](const Marker &m) { return m.name == name; }),
          voice->markers.end());
      return;
    }
  }
}

void AudioManager::ClearMarkers(VoiceID id) {
  for (size_t i = 0; i < pImpl->voicePool.GetVoiceCount(); ++i) {
    Voice *voice = pImpl->voicePool.GetVoiceAt(i);
    if (voice && voice->id == id) {
      voice->markers.clear();
      return;
    }
  }
}

// =============================================================================
// Interactive Music API
// =============================================================================

MusicManager &AudioManager::GetMusicManager() { return *pImpl->musicManager; }

// =============================================================================
// RTPC Curves API
// =============================================================================

void AudioManager::BindRTPC(const std::string &paramName,
                            const RTPCCurve &curve,
                            std::function<void(float)> callback) {
  // Create binding
  RTPCBinding binding;
  binding.parameterName = paramName;
  binding.curve = curve;
  binding.callback = std::move(callback);
  pImpl->rtpcBindings.push_back(binding);

  // Get or create parameter and bind to it
  Parameter *param = GetParam(paramName);
  if (param) {
    // Capture the binding index
    size_t bindingIndex = pImpl->rtpcBindings.size() - 1;
    param->Bind([this, bindingIndex](float value) {
      if (bindingIndex < pImpl->rtpcBindings.size()) {
        auto &b = pImpl->rtpcBindings[bindingIndex];
        float output = b.curve.Evaluate(value);
        if (b.callback) {
          b.callback(output);
        }
      }
    });
  }
}

void AudioManager::UnbindRTPC(const std::string &paramName) {
  pImpl->rtpcBindings.erase(
      std::remove_if(pImpl->rtpcBindings.begin(), pImpl->rtpcBindings.end(),
                     [&paramName](const RTPCBinding &b) {
                       return b.parameterName == paramName;
                     }),
      pImpl->rtpcBindings.end());
}

// =============================================================================
// Profiler API
// =============================================================================

AudioStats AudioManager::GetStats() const {
  AudioStats stats;

  // Voice counts from VoicePool
  stats.activeVoices = pImpl->voicePool.GetRealVoiceCount();
  stats.virtualVoices = pImpl->voicePool.GetVirtualVoiceCount();
  stats.totalVoices = pImpl->voicePool.GetActiveVoiceCount();
  stats.maxVoices = pImpl->voicePool.GetMaxVoices();

  // Engine stats
  stats.sampleRate =
      static_cast<uint32_t>(pImpl->engine.getBackendSamplerate());
  stats.bufferSize =
      static_cast<uint32_t>(pImpl->engine.getBackendBufferSize());
  stats.channels = static_cast<uint32_t>(pImpl->engine.getBackendChannels());

  // CPU usage estimate (based on active voice ratio)
  // This is a rough estimate; real CPU measurement would need platform-specific
  // code
  if (stats.maxVoices > 0) {
    stats.cpuUsage =
        (static_cast<float>(stats.activeVoices) / stats.maxVoices) * 100.0f;
  }

  // Memory estimate (rough: ~64KB per active voice for typical audio buffers)
  stats.memoryUsed = stats.activeVoices * 65536;

  return stats;
}

// =============================================================================
// Zone Crossfading API
// =============================================================================

void AudioManager::SetZoneCrossfadeEnabled(bool enabled) {
  pImpl->zoneCrossfadeEnabled = enabled;
}

bool AudioManager::IsZoneCrossfadeEnabled() const {
  return pImpl->zoneCrossfadeEnabled;
}

// =============================================================================
// Dynamic Zones API
// =============================================================================

AudioZone *AudioManager::GetZone(const std::string &eventName) {
  for (auto &zone : pImpl->zones) {
    if (zone->GetEventName() == eventName) {
      return zone.get();
    }
  }
  return nullptr;
}

void AudioManager::SetZonePosition(const std::string &eventName,
                                   const Vector3 &pos) {
  if (AudioZone *zone = GetZone(eventName)) {
    zone->SetPosition(pos);
  }
}

void AudioManager::SetZoneRadii(const std::string &eventName, float inner,
                                float outer) {
  if (AudioZone *zone = GetZone(eventName)) {
    zone->SetRadii(inner, outer);
  }
}

// =============================================================================
// Bus Compressor API
// =============================================================================

void AudioManager::SetBusCompressor(const std::string &busName,
                                    const CompressorSettings &settings) {
  auto busResult = GetBus(busName);
  if (busResult) {
    busResult.Value()->SetCompressor(settings);
  }
}

void AudioManager::SetBusCompressorEnabled(const std::string &busName,
                                           bool enabled) {
  auto busResult = GetBus(busName);
  if (busResult) {
    busResult.Value()->SetCompressorEnabled(enabled);
  }
}

void AudioManager::SetBusLimiter(const std::string &busName,
                                 float thresholdDb) {
  auto busResult = GetBus(busName);
  if (busResult) {
    CompressorSettings settings;
    settings.threshold = thresholdDb;
    settings.limiterMode = true;
    settings.attackMs = 1.0f;   // Fast attack for limiting
    settings.releaseMs = 50.0f; // Moderate release
    busResult.Value()->SetCompressor(settings);
    busResult.Value()->SetCompressorEnabled(true);
  }
}

// =============================================================================
// Convolution Reverb API
// =============================================================================

bool AudioManager::CreateConvolutionReverb(const std::string &name,
                                           const std::string &irPath) {
  auto reverb = std::make_unique<ConvolutionReverb>(44100.0f);
  if (!reverb->LoadImpulseResponse(irPath)) {
    return false;
  }
  pImpl->convolutionReverbs[name] = std::move(reverb);
  return true;
}

void AudioManager::SetConvolutionReverbWet(const std::string &name, float wet) {
  auto it = pImpl->convolutionReverbs.find(name);
  if (it != pImpl->convolutionReverbs.end()) {
    it->second->SetWet(wet);
  }
}

void AudioManager::SetConvolutionReverbEnabled(const std::string &name,
                                               bool enabled) {
  auto it = pImpl->convolutionReverbs.find(name);
  if (it != pImpl->convolutionReverbs.end()) {
    it->second->SetEnabled(enabled);
  }
}

ConvolutionReverb *AudioManager::GetConvolutionReverb(const std::string &name) {
  auto it = pImpl->convolutionReverbs.find(name);
  if (it != pImpl->convolutionReverbs.end()) {
    return it->second.get();
  }
  return nullptr;
}

// =============================================================================
// HDR Audio API
// =============================================================================

void AudioManager::SetTargetLoudness(float lufs) {
  pImpl->hdrMixer.SetTargetLoudness(lufs);
}

float AudioManager::GetTargetLoudness() const {
  return pImpl->hdrMixer.GetTargetLoudness();
}

void AudioManager::SetHDREnabled(bool enabled) {
  pImpl->hdrMixer.SetEnabled(enabled);
}

bool AudioManager::IsHDREnabled() const { return pImpl->hdrMixer.IsEnabled(); }

float AudioManager::GetMomentaryLUFS() const {
  return pImpl->hdrMixer.GetMomentaryLUFS();
}

float AudioManager::GetShortTermLUFS() const {
  return pImpl->hdrMixer.GetShortTermLUFS();
}

float AudioManager::GetTruePeakDB() const {
  return pImpl->hdrMixer.GetTruePeakDB();
}

// =============================================================================
// Surround Audio API
// =============================================================================

SpeakerLayout AudioManager::GetSpeakerLayout() const {
  unsigned int channels = pImpl->engine.getBackendChannels();
  return GetLayoutFromChannels(static_cast<int>(channels));
}

void AudioManager::SetVoiceSurroundGains(AudioHandle handle,
                                         const SpeakerGains &gains) {
  // SoLoud setPanAbsolute: L, R, LB, RB, C, S (for 5.1)
  // Our layout: FL, FR, C, LFE, SL, SR
  pImpl->engine.setPanAbsolute(handle,
                               gains.gains[0],  // Front Left
                               gains.gains[1],  // Front Right
                               gains.gains[4],  // Surround Left (as LB)
                               gains.gains[5],  // Surround Right (as RB)
                               gains.gains[2],  // Center
                               gains.gains[3]); // LFE (as S)
}

void AudioManager::SetVoiceLFEGain(AudioHandle handle, float lfeGain) {
  // Set pan with only LFE active
  // This is a simplified approach - full implementation would modify existing
  // gains
  (void)handle;
  (void)lfeGain;
  // Note: SoLoud doesn't have direct per-channel volume control post-pan
  // LFE routing requires combining with setPanAbsolute in SetVoiceSurroundGains
}

void AudioManager::SetVoiceCenterBias(AudioHandle handle, float centerBias) {
  (void)handle;
  (void)centerBias;
  // Note: Center bias is applied through SurroundPanner::ApplyCenterBias
  // before calling SetVoiceSurroundGains
}

// =============================================================================
// Ray-traced Acoustics API
// =============================================================================

void AudioManager::SetRayTracingEnabled(bool enabled) {
  pImpl->rayTracer.SetEnabled(enabled);
}

bool AudioManager::IsRayTracingEnabled() const {
  return pImpl->rayTracer.IsEnabled();
}

void AudioManager::SetRayCount(int count) {
  pImpl->rayTracer.SetRayCount(count);
}

void AudioManager::SetGeometryCallback(GeometryCallback callback) {
  pImpl->rayTracer.SetGeometryCallback(std::move(callback));
}

AcousticRayTracer &AudioManager::GetRayTracer() { return pImpl->rayTracer; }

// =============================================================================
// Audio Codec API
// =============================================================================

std::unique_ptr<AudioDecoder>
AudioManager::CreateDecoder(const std::string &path) {
  return DecoderFactory::CreateDecoder(path);
}

AudioCodec AudioManager::DetectCodec(const std::string &path) {
  return DecoderFactory::DetectCodec(path);
}

} // namespace Orpheus
