#pragma once

#include "AudioZone.h"
#include "Bus.h"
#include "Event.h"
#include "Listener.h"
#include "MixZone.h"
#include "OcclusionProcessor.h"
#include "Parameter.h"
#include "ReverbBus.h"
#include "ReverbZone.h"
#include "Snapshot.h"
#include "SoundBank.h"
#include "Types.h"
#include "VoicePool.h"

// Zone callbacks
using ZoneEnterCallback = std::function<void(const std::string &)>;
using ZoneExitCallback = std::function<void(const std::string &)>;

class AudioManager {
public:
  AudioManager();
  ~AudioManager();

  // Core lifecycle
  bool Init();
  void Shutdown();
  void Update(float dt);

  // Event playback
  VoiceID PlayEvent(const std::string &name, Vector3 position = {0, 0, 0});
  AudioHandle PlayEventDirect(const std::string &name);
  void RegisterEvent(const EventDescriptor &ed);
  bool RegisterEvent(const std::string &jsonString);
  bool LoadEventsFromFile(const std::string &jsonPath);

  // Parameters
  void SetGlobalParameter(const std::string &name, float value);
  Parameter *GetParam(const std::string &name);

  // Audio zones
  void AddAudioZone(const std::string &eventName, const Vector3 &pos,
                    float inner, float outer);
  void AddAudioZone(const std::string &eventName, const Vector3 &pos,
                    float inner, float outer, const std::string &snapshotName,
                    float fadeIn = 0.5f, float fadeOut = 0.5f);

  // Listener management
  ListenerID CreateListener();
  void DestroyListener(ListenerID id);
  void SetListenerPosition(ListenerID id, const Vector3 &pos);
  void SetListenerPosition(ListenerID id, float x, float y, float z);
  void SetListenerVelocity(ListenerID id, const Vector3 &vel);
  void SetListenerOrientation(ListenerID id, const Vector3 &forward,
                              const Vector3 &up);

  // Bus API
  void CreateBus(const std::string &name);
  std::shared_ptr<Bus> GetBus(const std::string &name);

  // Snapshot API
  void CreateSnapshot(const std::string &name);
  void SetSnapshotBusVolume(const std::string &snap, const std::string &bus,
                            float volume);
  void ApplySnapshot(const std::string &name, float fadeSeconds = 0.3f);
  void ResetBusVolumes(float fadeSeconds = 0.3f);
  void ResetEventVolume(const std::string &eventName, float fadeSeconds = 0.3f);

  // Voice Pool API
  void SetMaxVoices(uint32_t maxReal);
  uint32_t GetMaxVoices() const;
  void SetStealBehavior(StealBehavior behavior);
  StealBehavior GetStealBehavior() const;
  uint32_t GetActiveVoiceCount() const;
  uint32_t GetRealVoiceCount() const;
  uint32_t GetVirtualVoiceCount() const;

  // Mix Zone API
  void AddMixZone(const std::string &name, const std::string &snapshotName,
                  const Vector3 &pos, float inner, float outer,
                  uint8_t priority = 128, float fadeIn = 0.5f,
                  float fadeOut = 0.5f);
  void RemoveMixZone(const std::string &name);
  void SetZoneEnterCallback(ZoneEnterCallback cb);
  void SetZoneExitCallback(ZoneExitCallback cb);
  const std::string &GetActiveMixZone() const;

  // Reverb Bus API
  bool CreateReverbBus(const std::string &name, float roomSize = 0.5f,
                       float damp = 0.5f, float wet = 0.5f, float width = 1.0f);
  bool CreateReverbBus(const std::string &name, ReverbPreset preset);
  std::shared_ptr<ReverbBus> GetReverbBus(const std::string &name);
  void SetReverbParams(const std::string &name, float wet, float roomSize,
                       float damp, float fadeTime = 0.0f);
  void AddReverbZone(const std::string &name, const std::string &reverbBusName,
                     const Vector3 &pos, float inner, float outer,
                     uint8_t priority = 128);
  void RemoveReverbZone(const std::string &name);
  void SetSnapshotReverbParams(const std::string &snapshotName,
                               const std::string &reverbBusName, float wet,
                               float roomSize, float damp, float width = 1.0f);
  std::vector<std::string> GetActiveReverbZones() const;

  // Occlusion API
  void SetOcclusionQueryCallback(OcclusionQueryCallback callback);
  void RegisterOcclusionMaterial(const OcclusionMaterial &mat);
  void SetOcclusionEnabled(bool enabled);
  void SetOcclusionThreshold(float threshold);
  void SetOcclusionSmoothingTime(float seconds);
  void SetOcclusionUpdateRate(float hz);
  void SetOcclusionLowPassRange(float minFreq, float maxFreq);
  void SetOcclusionVolumeReduction(float maxReduction);
  bool IsOcclusionEnabled() const;

  // Engine access
  SoLoud::Soloud &Engine();

private:
  void UpdateMixZones(const Vector3 &listenerPos);
  void UpdateReverbZones(const Vector3 &listenerPos);

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

  // Mix zones
  std::vector<std::shared_ptr<MixZone>> m_MixZones;
  std::string m_ActiveMixZone;
  ZoneEnterCallback m_ZoneEnterCallback;
  ZoneExitCallback m_ZoneExitCallback;

  // Reverb system
  std::unordered_map<std::string, std::shared_ptr<ReverbBus>> m_ReverbBuses;
  std::vector<std::shared_ptr<ReverbZone>> m_ReverbZones;

  // Occlusion system
  OcclusionProcessor m_OcclusionProcessor;
};
