// AudioZone.h
#pragma once
#include "Types.h"

// Callback type for playing events - returns handle
using PlayEventCallback = std::function<AudioHandle(const std::string &)>;
using SetVolumeCallback = std::function<void(AudioHandle, float)>;
using StopCallback = std::function<void(AudioHandle)>;
using IsValidCallback = std::function<bool(AudioHandle)>;

class AudioZone {
public:
  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid)
      : m_EventName(eventName), m_Position(position),
        m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
        m_PlayEvent(playEvent), m_SetVolume(setVolume), m_Stop(stop),
        m_IsValid(isValid), m_Handle(0) {}

  void update(const Vector3 &listenerPos) {
    float dist = distance(listenerPos, m_Position);
    float vol = computeVolume(dist);

    if (vol > 0.0f) {
      // If not playing yet, start via event system
      if (m_Handle == 0 || !m_IsValid(m_Handle)) {
        m_Handle = m_PlayEvent(m_EventName);
      }
      if (m_Handle != 0) {
        m_SetVolume(m_Handle, vol);
      }
    } else {
      // Stop when too far
      if (m_Handle != 0 && m_IsValid(m_Handle)) {
        m_Stop(m_Handle);
        m_Handle = 0;
      }
    }
  }

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
};
