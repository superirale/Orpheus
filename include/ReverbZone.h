#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "Types.h"

namespace Orpheus {

class ReverbZone {
public:
  ReverbZone(const std::string &name, const std::string &reverbBusName,
             const Vector3 &position, float innerRadius, float outerRadius,
             uint8_t priority = 128);

  float Update(const Vector3 &listenerPos);

  float GetInfluence() const;
  bool IsActive() const;

  const std::string &GetName() const;
  const std::string &GetReverbBusName() const;
  const Vector3 &GetPosition() const;
  float GetInnerRadius() const;
  float GetOuterRadius() const;
  uint8_t GetPriority() const;
  float GetDistance(const Vector3 &listenerPos) const;

private:
  float Distance(const Vector3 &a, const Vector3 &b) const;
  float ComputeInfluence(float dist) const;

  std::string m_Name;
  std::string m_ReverbBusName;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  uint8_t m_Priority;
  float m_CurrentInfluence = 0.0f;
};

} // namespace Orpheus
