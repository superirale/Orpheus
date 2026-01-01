/**
 * @file ReverbZone.h
 * @brief Spatial zones for environment-based reverb.
 *
 * Provides ReverbZone class for automatically modulating reverb
 * send levels based on listener position.
 */
#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "Types.h"

namespace Orpheus {

/**
 * @brief Spatial zone that controls reverb bus influence.
 *
 * ReverbZone represents a spherical region that modulates the send
 * level to a reverb bus based on listener distance. This allows
 * automatic environment-aware reverb without per-sound configuration.
 *
 * @par Example:
 * A cathedral interior zone that increases reverb as the player
 * walks further inside the building.
 */
class ReverbZone {
public:
  /**
   * @brief Create a reverb zone.
   * @param name Unique name for this zone.
   * @param reverbBusName Name of the reverb bus to modulate.
   * @param position Center position in world space.
   * @param innerRadius Full influence within this distance.
   * @param outerRadius Influence fades to zero at this distance.
   * @param priority Higher priority zones take precedence (default: 128).
   */
  ReverbZone(const std::string &name, const std::string &reverbBusName,
             const Vector3 &position, float innerRadius, float outerRadius,
             uint8_t priority = 128);

  /**
   * @brief Update the zone based on listener position.
   * @param listenerPos Current listener position.
   * @return Current influence (0.0-1.0).
   */
  float Update(const Vector3 &listenerPos);

  /**
   * @brief Get the current influence factor.
   * @return Influence (0.0 = outside, 1.0 = fully inside).
   */
  float GetInfluence() const;

  /**
   * @brief Check if the zone is currently active.
   * @return true if influence is greater than 0.
   */
  bool IsActive() const;

  /**
   * @brief Get the zone name.
   * @return Reference to the zone name.
   */
  const std::string &GetName() const;

  /**
   * @brief Get the reverb bus name.
   * @return Reference to the reverb bus name.
   */
  const std::string &GetReverbBusName() const;

  /**
   * @brief Get the zone position.
   * @return Reference to the position.
   */
  const Vector3 &GetPosition() const;

  /**
   * @brief Get the inner radius.
   * @return Inner radius in world units.
   */
  float GetInnerRadius() const;

  /**
   * @brief Get the outer radius.
   * @return Outer radius in world units.
   */
  float GetOuterRadius() const;

  /**
   * @brief Get the zone priority.
   * @return Priority value (0-255).
   */
  uint8_t GetPriority() const;

  /**
   * @brief Get distance from zone center to listener.
   * @param listenerPos Listener position.
   * @return Distance in world units.
   */
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
