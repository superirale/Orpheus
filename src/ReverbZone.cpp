#include "../include/ReverbZone.h"

namespace Orpheus {

ReverbZone::ReverbZone(const std::string &name,
                       const std::string &reverbBusName,
                       const Vector3 &position, float innerRadius,
                       float outerRadius, uint8_t priority)
    : m_Name(name), m_ReverbBusName(reverbBusName), m_Position(position),
      m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
      m_Priority(priority) {}

float ReverbZone::Update(const Vector3 &listenerPos) {
  float dist = Distance(listenerPos, m_Position);
  m_CurrentInfluence = ComputeInfluence(dist);
  return m_CurrentInfluence;
}

float ReverbZone::GetInfluence() const { return m_CurrentInfluence; }
bool ReverbZone::IsActive() const { return m_CurrentInfluence > 0.0f; }

const std::string &ReverbZone::GetName() const { return m_Name; }
const std::string &ReverbZone::GetReverbBusName() const {
  return m_ReverbBusName;
}
const Vector3 &ReverbZone::GetPosition() const { return m_Position; }
float ReverbZone::GetInnerRadius() const { return m_InnerRadius; }
float ReverbZone::GetOuterRadius() const { return m_OuterRadius; }
uint8_t ReverbZone::GetPriority() const { return m_Priority; }

float ReverbZone::GetDistance(const Vector3 &listenerPos) const {
  return Distance(listenerPos, m_Position);
}

float ReverbZone::Distance(const Vector3 &a, const Vector3 &b) const {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  float dz = a.z - b.z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

float ReverbZone::ComputeInfluence(float dist) const {
  if (dist < m_InnerRadius)
    return 1.0f;
  if (dist > m_OuterRadius)
    return 0.0f;
  return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
}

} // namespace Orpheus
