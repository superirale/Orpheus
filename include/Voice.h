#pragma once

#include "Types.h"

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

  // Calculate audibility based on listener position
  void updateAudibility(const Vector3 &listenerPos) {
    float dx = position.x - listenerPos.x;
    float dy = position.y - listenerPos.y;
    float dz = position.z - listenerPos.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    float distAtten = 1.0f - std::min(dist / maxDistance, 1.0f);
    audibility = volume * std::max(distAtten, 0.0f);
  }

  // Get distance to listener
  float getDistance(const Vector3 &listenerPos) const {
    float dx = position.x - listenerPos.x;
    float dy = position.y - listenerPos.y;
    float dz = position.z - listenerPos.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
  }

  bool isReal() const { return state == VoiceState::Real; }
  bool isVirtual() const { return state == VoiceState::Virtual; }
  bool isStopped() const { return state == VoiceState::Stopped; }
};
