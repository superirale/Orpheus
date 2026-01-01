/**
 * @file DistanceCurve.h
 * @brief Distance attenuation curves for 3D audio.
 *
 * Provides different rolloff curves for controlling how sound
 * volume decreases with distance from the listener.
 */
#pragma once

#include <cmath>
#include <functional>

namespace Orpheus {

/**
 * @brief Distance attenuation curve types.
 */
enum class DistanceCurve {
  Linear,        ///< Linear rolloff: 1 - (dist / max)
  Logarithmic,   ///< Natural log-based rolloff (more realistic)
  InverseSquare, ///< Physics-based: 1 / (1 + dist²)
  Exponential,   ///< Exponential falloff
  Custom         ///< User-provided function
};

/**
 * @brief Settings for distance-based attenuation.
 */
struct DistanceSettings {
  DistanceCurve curve = DistanceCurve::Linear;
  float minDistance = 1.0f; ///< Distance where attenuation starts (full volume)
  float maxDistance = 100.0f; ///< Distance where sound becomes inaudible
  float rolloffFactor = 1.0f; ///< Multiplier for curve steepness

  /**
   * @brief Custom attenuation function.
   *
   * Takes normalized distance (0-1) and returns attenuation (0-1).
   * Only used when curve == DistanceCurve::Custom.
   */
  std::function<float(float)> customCurve;
};

/**
 * @brief Calculate attenuation based on distance and settings.
 *
 * @param distance Current distance from listener.
 * @param settings Distance attenuation settings.
 * @return Attenuation factor (0.0 = silent, 1.0 = full volume).
 */
inline float CalculateAttenuation(float distance,
                                  const DistanceSettings &settings) {
  // Clamp distance to valid range
  if (distance <= settings.minDistance) {
    return 1.0f;
  }
  if (distance >= settings.maxDistance) {
    return 0.0f;
  }

  // Normalize distance to 0-1 range
  float range = settings.maxDistance - settings.minDistance;
  float normalizedDist = (distance - settings.minDistance) / range;

  // Apply rolloff factor
  normalizedDist *= settings.rolloffFactor;
  normalizedDist = (std::min)(normalizedDist, 1.0f);

  float attenuation = 0.0f;

  switch (settings.curve) {
  case DistanceCurve::Linear:
    // Simple linear: 1 - dist
    attenuation = 1.0f - normalizedDist;
    break;

  case DistanceCurve::Logarithmic:
    // Logarithmic: more gradual at close range, faster falloff at distance
    // Formula: 1 - log10(1 + 9*dist) / log10(10) = 1 - log10(1 + 9*dist)
    attenuation = 1.0f - std::log10(1.0f + 9.0f * normalizedDist);
    break;

  case DistanceCurve::InverseSquare:
    // Physics-based inverse square law
    // Formula: 1 / (1 + dist²)
    attenuation = 1.0f / (1.0f + normalizedDist * normalizedDist * 4.0f);
    break;

  case DistanceCurve::Exponential:
    // Exponential falloff: e^(-dist*3)
    attenuation = std::exp(-normalizedDist * 3.0f);
    break;

  case DistanceCurve::Custom:
    if (settings.customCurve) {
      attenuation = settings.customCurve(normalizedDist);
    } else {
      attenuation = 1.0f - normalizedDist; // Fallback to linear
    }
    break;
  }

  return (std::max)(0.0f, (std::min)(1.0f, attenuation));
}

} // namespace Orpheus
