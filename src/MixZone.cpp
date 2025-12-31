#include "../include/MixZone.h"

namespace Orpheus {

MixZone::MixZone(const std::string &name, const std::string &snapshotName,
                 const Vector3 &position, float innerRadius, float outerRadius,
                 uint8_t priority, float fadeInTime, float fadeOutTime)
    : m_Name(name), m_SnapshotName(snapshotName), m_Position(position),
      m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
      m_Priority(priority), m_FadeInTime(fadeInTime),
      m_FadeOutTime(fadeOutTime) {}

float MixZone::Update(const Vector3 &listenerPos) {
  float dist = Distance(listenerPos, m_Position);
  float newBlend = ComputeBlend(dist);

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

bool MixZone::IsActive() const { return m_BlendFactor > 0.0f; }
float MixZone::GetBlendFactor() const { return m_BlendFactor; }
const std::string &MixZone::GetName() const { return m_Name; }
const std::string &MixZone::GetSnapshotName() const { return m_SnapshotName; }
uint8_t MixZone::GetPriority() const { return m_Priority; }
float MixZone::GetFadeInTime() const { return m_FadeInTime; }
float MixZone::GetFadeOutTime() const { return m_FadeOutTime; }

bool MixZone::JustEntered() const { return m_JustEntered; }
bool MixZone::JustExited() const { return m_JustExited; }

float MixZone::GetDistance(const Vector3 &listenerPos) const {
  return Distance(listenerPos, m_Position);
}

float MixZone::Distance(const Vector3 &a, const Vector3 &b) const {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  float dz = a.z - b.z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

float MixZone::ComputeBlend(float dist) const {
  if (dist < m_InnerRadius)
    return 1.0f;
  if (dist > m_OuterRadius)
    return 0.0f;
  return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
}

} // namespace Orpheus
