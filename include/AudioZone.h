/**
 * @file AudioZone.h
 * @brief Spatial audio zones for positional sound playback.
 *
 * Provides AudioZone class for playing sounds that attenuate with
 * distance and optionally trigger snapshots.
 */
#pragma once

#include <cmath>
#include <functional>
#include <string>

#include "Types.h"

namespace Orpheus {

/**
 * @brief Callback to play an event and return its handle.
 */
using PlayEventCallback = std::function<AudioHandle(const std::string &)>;

/**
 * @brief Callback to set volume on an audio handle.
 */
using SetVolumeCallback = std::function<void(AudioHandle, float)>;

/**
 * @brief Callback to stop an audio handle.
 */
using StopCallback = std::function<void(AudioHandle)>;

/**
 * @brief Callback to check if an audio handle is valid.
 */
using IsValidCallback = std::function<bool(AudioHandle)>;

/**
 * @brief Callback to apply a snapshot with fade.
 */
using AudioZoneApplySnapshotCallback =
    std::function<void(const std::string &, float)>;

/**
 * @brief Callback to revert from a snapshot with fade.
 */
using AudioZoneRevertSnapshotCallback = std::function<void(float)>;

/**
 * @brief Spatial audio zone for positional ambient sounds.
 *
 * AudioZone represents a spherical region that plays an audio event
 * when the listener enters. Volume is attenuated based on distance
 * with configurable inner/outer radii. Can optionally trigger a
 * snapshot when active.
 *
 * @par Example:
 * A waterfall zone where sound gets louder as you approach and the
 * mix shifts to emphasize ambient sounds.
 */
class AudioZone {
public:
  /**
   * @brief Create an audio zone without snapshot.
   * @param eventName Name of the event to play.
   * @param position Center position in world space.
   * @param innerRadius Full volume within this distance.
   * @param outerRadius Sound fades to zero at this distance.
   * @param playEvent Callback to play the event.
   * @param setVolume Callback to set handle volume.
   * @param stop Callback to stop the handle.
   * @param isValid Callback to check handle validity.
   */
  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid);

  /**
   * @brief Create an audio zone with snapshot support.
   * @param eventName Name of the event to play.
   * @param position Center position in world space.
   * @param innerRadius Full volume within this distance.
   * @param outerRadius Sound fades to zero at this distance.
   * @param playEvent Callback to play the event.
   * @param setVolume Callback to set handle volume.
   * @param stop Callback to stop the handle.
   * @param isValid Callback to check handle validity.
   * @param snapshotName Snapshot to apply when zone is active.
   * @param applySnapshot Callback to apply the snapshot.
   * @param revertSnapshot Callback to revert from snapshot.
   * @param fadeIn Fade-in time for snapshot (seconds).
   * @param fadeOut Fade-out time for snapshot (seconds).
   */
  AudioZone(const std::string &eventName, const Vector3 &position,
            float innerRadius, float outerRadius, PlayEventCallback playEvent,
            SetVolumeCallback setVolume, StopCallback stop,
            IsValidCallback isValid, const std::string &snapshotName,
            AudioZoneApplySnapshotCallback applySnapshot,
            AudioZoneRevertSnapshotCallback revertSnapshot, float fadeIn = 0.5f,
            float fadeOut = 0.5f);

  /**
   * @brief Update the zone based on listener position.
   * @param listenerPos Current listener position.
   */
  void Update(const Vector3 &listenerPos);

  /**
   * @brief Check if the zone is currently active.
   * @return true if listener is within outer radius.
   */
  [[nodiscard]] bool IsActive() const;

  /**
   * @brief Check if this zone has a snapshot attached.
   * @return true if a snapshot is configured.
   */
  [[nodiscard]] bool HasSnapshot() const;

  /**
   * @brief Get the snapshot name.
   * @return Reference to the snapshot name (empty if none).
   */
  [[nodiscard]] const std::string &GetSnapshotName() const;

  /**
   * @brief Get the event name.
   * @return Reference to the event name.
   */
  [[nodiscard]] const std::string &GetEventName() const;

  /**
   * @brief Get the zone position.
   * @return Reference to the position.
   */
  [[nodiscard]] const Vector3 &GetPosition() const;

private:
  float Distance(const Vector3 &a, const Vector3 &b);
  float ComputeVolume(float dist);

  std::string m_EventName;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  PlayEventCallback m_PlayEvent;
  SetVolumeCallback m_SetVolume;
  StopCallback m_Stop;
  IsValidCallback m_IsValid;
  AudioHandle m_Handle;

  std::string m_SnapshotName;
  AudioZoneApplySnapshotCallback m_ApplySnapshot;
  AudioZoneRevertSnapshotCallback m_RevertSnapshot;
  bool m_WasActive;
  float m_FadeInTime;
  float m_FadeOutTime;
};

} // namespace Orpheus
