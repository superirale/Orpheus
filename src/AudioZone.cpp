#include "../include/AudioZone.h"

namespace Orpheus {

AudioZone::AudioZone(const std::string &eventName, const Vector3 &position,
                     float innerRadius, float outerRadius,
                     PlayEventCallback playEvent, SetVolumeCallback setVolume,
                     StopCallback stop, IsValidCallback isValid)
    : m_EventName(eventName), m_Position(position), m_InnerRadius(innerRadius),
      m_OuterRadius(outerRadius), m_PlayEvent(playEvent),
      m_SetVolume(setVolume), m_Stop(stop), m_IsValid(isValid), m_Handle(0),
      m_WasActive(false), m_FadeInTime(0.5f), m_FadeOutTime(0.5f) {}

AudioZone::AudioZone(const std::string &eventName, const Vector3 &position,
                     float innerRadius, float outerRadius,
                     PlayEventCallback playEvent, SetVolumeCallback setVolume,
                     StopCallback stop, IsValidCallback isValid,
                     const std::string &snapshotName,
                     AudioZoneApplySnapshotCallback applySnapshot,
                     AudioZoneRevertSnapshotCallback revertSnapshot,
                     float fadeIn, float fadeOut)
    : m_EventName(eventName), m_Position(position), m_InnerRadius(innerRadius),
      m_OuterRadius(outerRadius), m_PlayEvent(playEvent),
      m_SetVolume(setVolume), m_Stop(stop), m_IsValid(isValid), m_Handle(0),
      m_SnapshotName(snapshotName), m_ApplySnapshot(applySnapshot),
      m_RevertSnapshot(revertSnapshot), m_WasActive(false),
      m_FadeInTime(fadeIn), m_FadeOutTime(fadeOut) {}

void AudioZone::Update(const Vector3 &listenerPos) {
  float dist = Distance(listenerPos, m_Position);
  float vol = ComputeVolume(dist);
  bool isActive = (vol > 0.0f);

  if (isActive) {
    if (m_Handle == 0 || !m_IsValid(m_Handle)) {
      m_Handle = m_PlayEvent(m_EventName);
    }
    if (m_Handle != 0) {
      m_SetVolume(m_Handle, vol);
    }

    if (!m_WasActive && HasSnapshot()) {
      m_ApplySnapshot(m_SnapshotName, m_FadeInTime);
    }
  } else {
    if (m_Handle != 0 && m_IsValid(m_Handle)) {
      m_Stop(m_Handle);
      m_Handle = 0;
    }

    if (m_WasActive && HasSnapshot()) {
      m_RevertSnapshot(m_FadeOutTime);
    }
  }

  m_WasActive = isActive;
}

bool AudioZone::IsActive() const { return m_WasActive; }

bool AudioZone::HasSnapshot() const {
  return !m_SnapshotName.empty() && m_ApplySnapshot;
}

const std::string &AudioZone::GetSnapshotName() const { return m_SnapshotName; }
const std::string &AudioZone::GetEventName() const { return m_EventName; }
const Vector3 &AudioZone::GetPosition() const { return m_Position; }

float AudioZone::GetComputedVolume(const Vector3 &listenerPos) const {
  float dist = Distance(listenerPos, m_Position);
  return ComputeVolumeFromDist(dist);
}

void AudioZone::ApplyVolume(float volume) {
  if (m_Handle != 0 && m_IsValid(m_Handle)) {
    m_SetVolume(m_Handle, volume);
  }
}

void AudioZone::EnsurePlaying() {
  if (m_Handle == 0 || !m_IsValid(m_Handle)) {
    m_Handle = m_PlayEvent(m_EventName);
  }

  if (!m_WasActive && HasSnapshot()) {
    m_ApplySnapshot(m_SnapshotName, m_FadeInTime);
  }
  m_WasActive = true;
}

void AudioZone::StopPlaying() {
  if (m_Handle != 0 && m_IsValid(m_Handle)) {
    m_Stop(m_Handle);
    m_Handle = 0;
  }

  if (m_WasActive && HasSnapshot()) {
    m_RevertSnapshot(m_FadeOutTime);
  }
  m_WasActive = false;
}

float AudioZone::Distance(const Vector3 &a, const Vector3 &b) const {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  float dz = a.z - b.z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

float AudioZone::ComputeVolume(float dist) {
  return ComputeVolumeFromDist(dist);
}

float AudioZone::ComputeVolumeFromDist(float dist) const {
  if (dist < m_InnerRadius)
    return 1.0f;
  if (dist > m_OuterRadius)
    return 0.0f;
  return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
}

} // namespace Orpheus
