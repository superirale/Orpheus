// ReverbZone.h
#pragma once

#include "Types.h"

// A reverb zone defines a spatial region that influences reverb sends.
// When the listener is within the zone, sounds are sent to the associated
// reverb bus with a level based on distance.
class ReverbZone {
public:
  ReverbZone(const std::string &name, const std::string &reverbBusName,
             const Vector3 &position, float innerRadius, float outerRadius,
             uint8_t priority = 128)
      : m_Name(name), m_ReverbBusName(reverbBusName), m_Position(position),
        m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
        m_Priority(priority) {}

  // Update zone influence based on listener position
  // Returns the send level (0.0-1.0) for this zone
  float Update(const Vector3 &listenerPos) {
    float dist = Distance(listenerPos, m_Position);
    m_CurrentInfluence = ComputeInfluence(dist);
    return m_CurrentInfluence;
  }

  // Get the current send level for this zone (0.0-1.0)
  float GetInfluence() const { return m_CurrentInfluence; }

  // Check if the zone is currently active (listener within outer radius)
  bool IsActive() const { return m_CurrentInfluence > 0.0f; }

  // Getters
  const std::string &GetName() const { return m_Name; }
  const std::string &GetReverbBusName() const { return m_ReverbBusName; }
  const Vector3 &GetPosition() const { return m_Position; }
  float GetInnerRadius() const { return m_InnerRadius; }
  float GetOuterRadius() const { return m_OuterRadius; }
  uint8_t GetPriority() const { return m_Priority; }

  // Get distance from listener to zone center
  float GetDistance(const Vector3 &listenerPos) const {
    return Distance(listenerPos, m_Position);
  }

private:
  float Distance(const Vector3 &a, const Vector3 &b) const {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
  }

  // Compute influence based on distance:
  // - Inside inner radius: full influence (1.0)
  // - Between inner and outer: linear falloff
  // - Outside outer radius: no influence (0.0)
  float ComputeInfluence(float dist) const {
    if (dist < m_InnerRadius)
      return 1.0f;
    if (dist > m_OuterRadius)
      return 0.0f;
    return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
  }

  std::string m_Name;
  std::string m_ReverbBusName;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  uint8_t m_Priority;

  // Current state
  float m_CurrentInfluence = 0.0f;
};
