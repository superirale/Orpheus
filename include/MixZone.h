/**
 * @file MixZone.h
 * @brief Mix zones for location-based snapshot blending.
 *
 * Provides MixZone class for triggering snapshot transitions
 * as the listener moves through the game world.
 */
#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "Types.h"

namespace Orpheus {

/**
 * @brief Spatial zone that triggers mix snapshot blending.
 *
 * MixZone represents a spherical region that applies a snapshot when
 * the listener enters. The blend factor is calculated based on distance,
 * allowing smooth transitions between mix states (e.g., indoor/outdoor,
 * calm/combat areas).
 *
 * @par Example:
 * A cave entrance zone that gradually applies a "cave" reverb snapshot
 * as the player walks deeper inside.
 */
class MixZone {
public:
  /**
   * @brief Create a mix zone.
   * @param name Unique name for this zone.
   * @param snapshotName Snapshot to apply when active.
   * @param position Center position in world space.
   * @param innerRadius Full blend within this distance.
   * @param outerRadius Blend fades to zero at this distance.
   * @param priority Higher priority zones take precedence (default: 128).
   * @param fadeInTime Snapshot fade-in duration in seconds.
   * @param fadeOutTime Snapshot fade-out duration in seconds.
   */
  MixZone(const std::string &name, const std::string &snapshotName,
          const Vector3 &position, float innerRadius, float outerRadius,
          uint8_t priority = 128, float fadeInTime = 0.5f,
          float fadeOutTime = 0.5f);

  /**
   * @brief Update the zone based on listener position.
   * @param listenerPos Current listener position.
   * @return Current blend factor (0.0-1.0).
   */
  float Update(const Vector3 &listenerPos);

  /**
   * @brief Check if the zone is currently active.
   * @return true if listener is within outer radius.
   */
  bool IsActive() const;

  /**
   * @brief Get the current blend factor.
   * @return Blend factor (0.0 = outside, 1.0 = fully inside).
   */
  float GetBlendFactor() const;

  /**
   * @brief Get the zone name.
   * @return Reference to the zone name.
   */
  const std::string &GetName() const;

  /**
   * @brief Get the snapshot name.
   * @return Reference to the snapshot name.
   */
  const std::string &GetSnapshotName() const;

  /**
   * @brief Get the zone priority.
   * @return Priority value (0-255).
   */
  uint8_t GetPriority() const;

  /**
   * @brief Get the fade-in time.
   * @return Fade-in duration in seconds.
   */
  float GetFadeInTime() const;

  /**
   * @brief Get the fade-out time.
   * @return Fade-out duration in seconds.
   */
  float GetFadeOutTime() const;

  /**
   * @brief Check if the listener just entered this frame.
   * @return true if just entered.
   */
  bool JustEntered() const;

  /**
   * @brief Check if the listener just exited this frame.
   * @return true if just exited.
   */
  bool JustExited() const;

  /**
   * @brief Get distance from zone center to listener.
   * @param listenerPos Listener position.
   * @return Distance in world units.
   */
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

} // namespace Orpheus
