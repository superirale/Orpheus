#pragma once

#include "Types.h"

// Callback types for zone events
using ZoneEnterCallback = std::function<void(const std::string &zoneName)>;
using ZoneExitCallback = std::function<void(const std::string &zoneName)>;
using ApplySnapshotCallback = std::function<void(
    const std::string &snapshotName, float intensity, float fadeSeconds)>;

class MixZone {
public:
  MixZone(const std::string &name, const std::string &snapshotName,
          const Vector3 &position, float innerRadius, float outerRadius,
          uint8_t priority = 128, float fadeInTime = 0.5f,
          float fadeOutTime = 0.5f)
      : m_Name(name), m_SnapshotName(snapshotName), m_Position(position),
        m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
        m_Priority(priority), m_FadeInTime(fadeInTime),
        m_FadeOutTime(fadeOutTime) {}

  // Update zone and return blend factor (0.0 = outside, 1.0 = fully inside)
  float update(const Vector3 &listenerPos) {
    float dist = distance(listenerPos, m_Position);
    float newBlend = computeBlend(dist);

    // Track previous state for enter/exit detection
    bool wasActive = m_BlendFactor > 0.0f;
    bool isNowActive = newBlend > 0.0f;

    if (!wasActive && isNowActive) {
      m_JustEntered = true;
      m_JustExited = false;
    } else if (wasActive && !isNowActive) {
      m_JustExited = true;
      m_JustEntered = false;
    } else {
      m_JustEntered = false;
      m_JustExited = false;
    }

    m_BlendFactor = newBlend;
    return m_BlendFactor;
  }

  // Getters
  bool isActive() const { return m_BlendFactor > 0.0f; }
  float getBlendFactor() const { return m_BlendFactor; }
  const std::string &getName() const { return m_Name; }
  const std::string &getSnapshotName() const { return m_SnapshotName; }
  uint8_t getPriority() const { return m_Priority; }
  float getFadeInTime() const { return m_FadeInTime; }
  float getFadeOutTime() const { return m_FadeOutTime; }

  // State change detection
  bool justEntered() const { return m_JustEntered; }
  bool justExited() const { return m_JustExited; }

  // Get distance to zone center
  float getDistance(const Vector3 &listenerPos) const {
    return distance(listenerPos, m_Position);
  }

private:
  float distance(const Vector3 &a, const Vector3 &b) const {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
  }

  float computeBlend(float dist) const {
    if (dist < m_InnerRadius)
      return 1.0f;
    if (dist > m_OuterRadius)
      return 0.0f;
    return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
  }

  std::string m_Name;
  std::string m_SnapshotName;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  uint8_t m_Priority;
  float m_FadeInTime;
  float m_FadeOutTime;
  float m_BlendFactor = 0.0f;
  bool m_JustEntered = false;
  bool m_JustExited = false;
};
