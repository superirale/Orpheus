// AudioZone.h
#pragma once
#include "Types.h"

using PlayEventCallback = std::function<AudioHandle(const std::string &)>;
using SetVolumeCallback = std::function<void(AudioHandle, float)>;
using StopCallback = std::function<void(AudioHandle)>;
using IsValidCallback = std::function<bool(AudioHandle)>;
using AudioZoneApplySnapshotCallback =
    std::function<void(const std::string &, float)>;
using AudioZoneRevertSnapshotCallback = std::function<void(float)>;

class AudioZone {
public:
  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid);

  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid, const std::string &snapshotName,
            AudioZoneApplySnapshotCallback applySnapshot,
            AudioZoneRevertSnapshotCallback revertSnapshot, float fadeIn = 0.5f,
            float fadeOut = 0.5f);

  void Update(const Vector3 &listenerPos);

  bool IsActive() const;
  bool HasSnapshot() const;
  const std::string &GetSnapshotName() const;
  const std::string &GetEventName() const;
  const Vector3 &GetPosition() const;

private:
  float Distance(const Vector3 &a, const Vector3 &b);
  float ComputeVolume(float dist);

  std::string m_EventName;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  PlayEventCallback m_PlayEvent;
  SetVolumeCallback m_SetVolume;
  StopCallback m_Stop;
  IsValidCallback m_IsValid;
  AudioHandle m_Handle;

  std::string m_SnapshotName;
  AudioZoneApplySnapshotCallback m_ApplySnapshot;
  AudioZoneRevertSnapshotCallback m_RevertSnapshot;
  bool m_WasActive;
  float m_FadeInTime;
  float m_FadeOutTime;
};
