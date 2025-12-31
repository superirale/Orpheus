#pragma once

#include "Types.h"

class MixZone {
public:
  MixZone(const std::string &name, const std::string &snapshotName,
          const Vector3 &position, float innerRadius, float outerRadius,
          uint8_t priority = 128, float fadeInTime = 0.5f,
          float fadeOutTime = 0.5f);

  float Update(const Vector3 &listenerPos);

  bool IsActive() const;
  float GetBlendFactor() const;
  const std::string &GetName() const;
  const std::string &GetSnapshotName() const;
  uint8_t GetPriority() const;
  float GetFadeInTime() const;
  float GetFadeOutTime() const;

  bool JustEntered() const;
  bool JustExited() const;
  float GetDistance(const Vector3 &listenerPos) const;

private:
  float Distance(const Vector3 &a, const Vector3 &b) const;
  float ComputeBlend(float dist) const;

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
