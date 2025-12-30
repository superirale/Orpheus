// AudioZone.h
#pragma once
#include "Types.h"

// Callback type for playing events - returns handle
using PlayEventCallback = std::function<AudioHandle(const std::string &)>;
using SetVolumeCallback = std::function<void(AudioHandle, float)>;
using StopCallback = std::function<void(AudioHandle)>;
using IsValidCallback = std::function<bool(AudioHandle)>;

// Snapshot callbacks for zone-triggered snapshots (AudioZone-specific)
using AudioZoneApplySnapshotCallback =
    std::function<void(const std::string &, float)>;
using AudioZoneRevertSnapshotCallback = std::function<void(float)>;

class AudioZone {
public:
  // Constructor without snapshot (backward compatible)
  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid)
      : m_EventName(eventName), m_Position(position),
        m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
        m_PlayEvent(playEvent), m_SetVolume(setVolume), m_Stop(stop),
        m_IsValid(isValid), m_Handle(0), m_WasActive(false), m_FadeInTime(0.5f),
        m_FadeOutTime(0.5f) {}

  // Constructor with snapshot binding
  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid, const std::string &snapshotName,
            AudioZoneApplySnapshotCallback applySnapshot,
            AudioZoneRevertSnapshotCallback revertSnapshot, float fadeIn = 0.5f,
            float fadeOut = 0.5f)
      : m_EventName(eventName), m_Position(position),
        m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
        m_PlayEvent(playEvent), m_SetVolume(setVolume), m_Stop(stop),
        m_IsValid(isValid), m_Handle(0), m_SnapshotName(snapshotName),
        m_ApplySnapshot(applySnapshot), m_RevertSnapshot(revertSnapshot),
        m_WasActive(false), m_FadeInTime(fadeIn), m_FadeOutTime(fadeOut) {}

  void update(const Vector3 &listenerPos) {
    float dist = distance(listenerPos, m_Position);
    float vol = computeVolume(dist);
    bool isActive = (vol > 0.0f);

    if (isActive) {
      // If not playing yet, start via event system
      if (m_Handle == 0 || !m_IsValid(m_Handle)) {
        m_Handle = m_PlayEvent(m_EventName);
      }
      if (m_Handle != 0) {
        m_SetVolume(m_Handle, vol);
      }

      // Zone entered - apply snapshot
      if (!m_WasActive && hasSnapshot()) {
        m_ApplySnapshot(m_SnapshotName, m_FadeInTime);
      }
    } else {
      // Stop when too far
      if (m_Handle != 0 && m_IsValid(m_Handle)) {
        m_Stop(m_Handle);
        m_Handle = 0;
      }

      // Zone exited - revert snapshot
      if (m_WasActive && hasSnapshot()) {
        m_RevertSnapshot(m_FadeOutTime);
      }
    }

    m_WasActive = isActive;
  }

  // Accessors
  bool isActive() const { return m_WasActive; }
  bool hasSnapshot() const {
    return !m_SnapshotName.empty() && m_ApplySnapshot;
  }
  const std::string &getSnapshotName() const { return m_SnapshotName; }
  const std::string &getEventName() const { return m_EventName; }
  const Vector3 &getPosition() const { return m_Position; }

private:
  float distance(const Vector3 &a, const Vector3 &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
  }

  float computeVolume(float dist) {
    if (dist < m_InnerRadius)
      return 1.0f;
    if (dist > m_OuterRadius)
      return 0.0f;
    return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
  }

  std::string m_EventName;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  PlayEventCallback m_PlayEvent;
  SetVolumeCallback m_SetVolume;
  StopCallback m_Stop;
  IsValidCallback m_IsValid;
  AudioHandle m_Handle;

  // Snapshot binding
  std::string m_SnapshotName;
  AudioZoneApplySnapshotCallback m_ApplySnapshot;
  AudioZoneRevertSnapshotCallback m_RevertSnapshot;
  bool m_WasActive;
  float m_FadeInTime;
  float m_FadeOutTime;
};
