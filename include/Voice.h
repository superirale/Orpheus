#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "Types.h"

namespace Orpheus {

// Voice state for virtual voice system
enum class VoiceState { Real, Virtual, Stopped };

// Behavior when stealing voices
enum class StealBehavior { Oldest, Furthest, Quietest, None };

using VoiceID = uint32_t;

struct Voice {
  VoiceID id = 0;
  std::string eventName;
  AudioHandle handle = 0; // SoLoud handle (0 if virtual)
  VoiceState state = VoiceState::Stopped;

  // Priority (0-255, higher = more important, never stolen by lower)
  uint8_t priority = 128;

  // 3D positioning
  Vector3 position{0, 0, 0};
  float maxDistance = 100.0f;

  // Volume and audibility
  float volume = 1.0f;
  float audibility = 1.0f; // Calculated: volume * distance attenuation

  // Time tracking
  float playbackTime = 0.0f; // Seconds since start (for resuming virtual)
  float startTime = 0.0f;    // When started (for Oldest stealing)

  // Reverb sends: bus name -> send level (0.0-1.0)
  // Updated each frame based on reverb zones
  std::unordered_map<std::string, float> reverbSends;

  // Occlusion state (calculated per update by OcclusionProcessor)
  float obstruction = 0.0f;            // 0.0-1.0 partial blocking
  float occlusion = 0.0f;              // 0.0-1.0 full blocking (derived)
  float occlusionSmoothed = 0.0f;      // Smoothed value for DSP
  float targetLowPassFreq = 22000.0f;  // Target filter cutoff Hz
  float currentLowPassFreq = 22000.0f; // Current (smoothed) cutoff Hz
  float occlusionVolume = 1.0f;        // Volume modifier from occlusion

  // Calculate audibility based on listener position
  void UpdateAudibility(const Vector3 &listenerPos) {
    float dx = position.x - listenerPos.x;
    float dy = position.y - listenerPos.y;
    float dz = position.z - listenerPos.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    float distAtten = 1.0f - std::min(dist / maxDistance, 1.0f);
    audibility = volume * std::max(distAtten, 0.0f);
  }

  // Get distance to listener
  float GetDistance(const Vector3 &listenerPos) const {
    float dx = position.x - listenerPos.x;
    float dy = position.y - listenerPos.y;
    float dz = position.z - listenerPos.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  bool IsReal() const { return state == VoiceState::Real; }
  bool IsVirtual() const { return state == VoiceState::Virtual; }
  bool IsStopped() const { return state == VoiceState::Stopped; }
};

} // namespace Orpheus
