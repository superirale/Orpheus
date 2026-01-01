/**
 * @file Voice.h
 * @brief Voice management for audio playback instances.
 *
 * Defines voice state, steal behavior, and the Voice struct for
 * managing active and virtual audio instances.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "DistanceCurve.h"
#include "Types.h"

namespace Orpheus {

/**
 * @brief Voice state for virtual voice system.
 */
enum class VoiceState {
  Real,    ///< Playing on hardware
  Virtual, ///< Tracked but not playing (out of voice limit)
  Stopped  ///< Finished or stopped
};

/**
 * @brief Behavior when stealing voices due to voice limit.
 */
enum class StealBehavior {
  Oldest,   ///< Steal the oldest playing voice
  Furthest, ///< Steal the furthest voice from listener
  Quietest, ///< Steal the quietest voice
  None      ///< Don't steal, fail allocation instead
};

/**
 * @brief Unique identifier for voices.
 */
using VoiceID = uint32_t;

/**
 * @brief Audio marker for time-based callbacks.
 *
 * Markers are triggered when playback reaches their specified time position.
 */
struct Marker {
  float time = 0.0f;              ///< Position in seconds
  std::string name;               ///< Optional identifier for removal
  std::function<void()> callback; ///< Callback to invoke
  bool triggered = false;         ///< Track if already fired this play
};

/**
 * @brief Represents an active or virtual audio playback instance.
 *
 * Tracks all information needed to manage a playing sound, including
 * its 3D position, priority, occlusion state, and reverb sends.
 */
struct Voice {
  VoiceID id = 0;                         ///< Unique voice identifier
  std::string eventName;                  ///< Name of the event being played
  AudioHandle handle = 0;                 ///< SoLoud handle (0 if virtual)
  VoiceState state = VoiceState::Stopped; ///< Current voice state

  /// @name Priority
  /// @{
  uint8_t priority = 128; ///< Priority (0-255, higher = more important)
  /// @}

  /// @name 3D Positioning
  /// @{
  Vector3 position{0, 0, 0};         ///< Position in world space
  Vector3 velocity{0, 0, 0};         ///< Velocity in world space (for Doppler)
  DistanceSettings distanceSettings; ///< Distance attenuation settings
  float dopplerPitch = 1.0f;         ///< Calculated Doppler pitch multiplier
  /// @}

  /// @name Volume and Audibility
  /// @{
  float volume = 1.0f;     ///< Base volume
  float audibility = 1.0f; ///< Calculated: volume * distance attenuation
  /// @}

  /// @name Time Tracking
  /// @{
  float playbackTime = 0.0f; ///< Seconds since start (for resuming virtual)
  float startTime = 0.0f;    ///< When started (for Oldest stealing)
  /// @}

  /// @name Reverb
  /// @{
  /**
   * @brief Reverb sends: bus name -> send level (0.0-1.0).
   *
   * Updated each frame based on reverb zones.
   */
  std::unordered_map<std::string, float> reverbSends;
  /// @}

  /// @name Occlusion
  /// @{
  float obstruction = 0.0f;            ///< 0.0-1.0 partial blocking
  float occlusion = 0.0f;              ///< 0.0-1.0 full blocking (derived)
  float occlusionSmoothed = 0.0f;      ///< Smoothed value for DSP
  float targetLowPassFreq = 22000.0f;  ///< Target filter cutoff Hz
  float currentLowPassFreq = 22000.0f; ///< Current (smoothed) cutoff Hz
  float occlusionVolume = 1.0f;        ///< Volume modifier from occlusion
  /// @}

  /// @name Markers
  /// @{
  std::vector<Marker> markers; ///< Time-based callback markers
  /// @}

  /**
   * @brief Calculate audibility based on listener position.
   * @param listenerPos Listener position in world space.
   */
  void UpdateAudibility(const Vector3 &listenerPos) {
    float dist = GetDistance(listenerPos);
    float distAtten = CalculateAttenuation(dist, distanceSettings);
    audibility = volume * distAtten;
  }

  /**
   * @brief Get distance to listener.
   * @param listenerPos Listener position in world space.
   * @return Distance in world units.
   */
  [[nodiscard]] float GetDistance(const Vector3 &listenerPos) const {
    float dx = position.x - listenerPos.x;
    float dy = position.y - listenerPos.y;
    float dz = position.z - listenerPos.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  /**
   * @brief Check if voice is real (playing on hardware).
   * @return true if state is Real.
   */
  [[nodiscard]] bool IsReal() const { return state == VoiceState::Real; }

  /**
   * @brief Check if voice is virtual (tracked but not playing).
   * @return true if state is Virtual.
   */
  [[nodiscard]] bool IsVirtual() const { return state == VoiceState::Virtual; }

  /**
   * @brief Check if voice is stopped.
   * @return true if state is Stopped.
   */
  [[nodiscard]] bool IsStopped() const { return state == VoiceState::Stopped; }
};

} // namespace Orpheus
