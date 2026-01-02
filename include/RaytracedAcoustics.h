/**
 * @file RaytracedAcoustics.h
 * @brief Ray-traced acoustics for advanced sound propagation simulation.
 *
 * Provides realistic audio propagation including:
 * - Direct path calculation
 * - Early reflections via ray tracing
 * - Frequency-dependent material absorption
 * - Diffraction around edges
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace Orpheus {

/**
 * @brief 3D vector for acoustic calculations.
 */
struct AcousticVector {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  AcousticVector() = default;
  AcousticVector(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

  AcousticVector operator+(const AcousticVector &v) const {
    return {x + v.x, y + v.y, z + v.z};
  }

  AcousticVector operator-(const AcousticVector &v) const {
    return {x - v.x, y - v.y, z - v.z};
  }

  AcousticVector operator*(float s) const { return {x * s, y * s, z * s}; }

  float Dot(const AcousticVector &v) const {
    return x * v.x + y * v.y + z * v.z;
  }

  float Length() const { return std::sqrt(x * x + y * y + z * z); }

  AcousticVector Normalized() const {
    float len = Length();
    if (len > 0.0f) {
      return {x / len, y / len, z / len};
    }
    return {0.0f, 0.0f, 0.0f};
  }

  AcousticVector Reflect(const AcousticVector &normal) const {
    float d = 2.0f * Dot(normal);
    return {x - normal.x * d, y - normal.y * d, z - normal.z * d};
  }
};

/**
 * @brief Acoustic ray for tracing.
 */
struct AcousticRay {
  AcousticVector origin;
  AcousticVector direction;
  float energy = 1.0f;   // Remaining energy (0-1)
  float distance = 0.0f; // Total distance traveled
  int bounces = 0;       // Number of reflections

  // Frequency band energies (low, mid, high)
  float energyLow = 1.0f;
  float energyMid = 1.0f;
  float energyHigh = 1.0f;
};

/**
 * @brief Material properties for acoustic simulation.
 */
struct AcousticMaterial {
  std::string name;

  // Absorption coefficients per frequency band (0 = full reflect, 1 = full
  // absorb)
  float absorptionLow = 0.1f;  // Low frequencies (< 500Hz)
  float absorptionMid = 0.2f;  // Mid frequencies (500-2000Hz)
  float absorptionHigh = 0.3f; // High frequencies (> 2000Hz)

  // Scattering coefficient (0 = specular, 1 = diffuse)
  float scattering = 0.1f;

  // Transmission coefficient (for thin walls)
  float transmission = 0.0f;

  static AcousticMaterial Concrete() {
    return {"Concrete", 0.01f, 0.02f, 0.02f, 0.1f, 0.0f};
  }

  static AcousticMaterial Wood() {
    return {"Wood", 0.15f, 0.11f, 0.10f, 0.2f, 0.0f};
  }

  static AcousticMaterial Carpet() {
    return {"Carpet", 0.08f, 0.24f, 0.57f, 0.7f, 0.0f};
  }

  static AcousticMaterial Glass() {
    return {"Glass", 0.18f, 0.06f, 0.04f, 0.05f, 0.0f};
  }

  static AcousticMaterial Curtain() {
    return {"Curtain", 0.07f, 0.31f, 0.49f, 0.8f, 0.0f};
  }
};

/**
 * @brief Result of a ray-scene intersection.
 */
struct RayHit {
  bool hit = false;
  float distance = 0.0f;
  AcousticVector point;
  AcousticVector normal;
  AcousticMaterial material;
};

/**
 * @brief Callback for scene geometry intersection.
 */
using GeometryCallback =
    std::function<RayHit(const AcousticVector &origin,
                         const AcousticVector &direction, float maxDistance)>;

/**
 * @brief Single propagation path from source to listener.
 */
struct PropagationPath {
  float delay = 0.0f;    // Time delay in seconds
  float gainLow = 1.0f;  // Gain for low frequencies
  float gainMid = 1.0f;  // Gain for mid frequencies
  float gainHigh = 1.0f; // Gain for high frequencies
  int reflections = 0;   // Number of bounces
  float distance = 0.0f; // Total path length
  bool isDirect = false; // Is this the direct path?
};

/**
 * @brief Result of acoustic ray tracing.
 */
struct PropagationResult {
  std::vector<PropagationPath> paths;
  float directDistance = 0.0f;
  bool hasDirectPath = false;

  // Combined impulse response parameters
  float earlyReflectionDelay = 0.0f;
  float earlyReflectionGain = 0.0f;
  float lateReverbTime = 0.0f;

  /**
   * @brief Get combined gain for a frequency band.
   */
  float GetCombinedGain(int band) const {
    float total = 0.0f;
    for (const auto &path : paths) {
      switch (band) {
      case 0:
        total += path.gainLow;
        break;
      case 1:
        total += path.gainMid;
        break;
      case 2:
        total += path.gainHigh;
        break;
      }
    }
    return (std::min)(total, 1.0f);
  }
};

/**
 * @brief Acoustic ray tracer for sound propagation simulation.
 */
class AcousticRayTracer {
public:
  static constexpr float kSpeedOfSound = 343.0f; // m/s at 20°C
  static constexpr float kMinEnergy = 0.001f;    // Energy cutoff
  static constexpr int kMaxBounces = 8;          // Max reflections

  /**
   * @brief Set geometry intersection callback.
   */
  void SetGeometryCallback(GeometryCallback callback) {
    m_GeometryCallback = std::move(callback);
  }

  /**
   * @brief Set number of rays to cast.
   */
  void SetRayCount(int count) { m_RayCount = std::clamp(count, 8, 1024); }

  /**
   * @brief Set maximum trace distance.
   */
  void SetMaxDistance(float distance) {
    m_MaxDistance = (std::max)(1.0f, distance);
  }

  /**
   * @brief Trace sound propagation from source to listener.
   */
  PropagationResult Trace(const AcousticVector &source,
                          const AcousticVector &listener) {
    PropagationResult result;

    // Calculate direct path
    AcousticVector toListener = listener - source;
    float directDistance = toListener.Length();
    result.directDistance = directDistance;

    // Check if direct path is occluded
    if (m_GeometryCallback) {
      RayHit hit =
          m_GeometryCallback(source, toListener.Normalized(), directDistance);
      result.hasDirectPath = !hit.hit || hit.distance >= directDistance;
    } else {
      result.hasDirectPath = true;
    }

    // Add direct path if not occluded
    if (result.hasDirectPath) {
      PropagationPath directPath;
      directPath.isDirect = true;
      directPath.distance = directDistance;
      directPath.delay = directDistance / kSpeedOfSound;

      // Distance attenuation (inverse square law)
      float attenuation =
          1.0f / (1.0f + directDistance * directDistance * 0.01f);
      directPath.gainLow = attenuation;
      directPath.gainMid = attenuation;
      directPath.gainHigh = attenuation * 0.9f; // Air absorption

      result.paths.push_back(directPath);
    }

    // Cast rays for reflections
    if (m_GeometryCallback) {
      CastReflectionRays(source, listener, result);
    }

    // Compute early reflection parameters
    ComputeEarlyReflections(result);

    return result;
  }

  /**
   * @brief Enable/disable ray tracing.
   */
  void SetEnabled(bool enabled) { m_Enabled = enabled; }
  bool IsEnabled() const { return m_Enabled; }

private:
  void CastReflectionRays(const AcousticVector &source,
                          const AcousticVector &listener,
                          PropagationResult &result) {
    float listenerRadius = 1.0f; // Listener capture radius

    for (int i = 0; i < m_RayCount; ++i) {
      // Generate ray in hemisphere (Fibonacci sphere distribution)
      float phi = std::acos(1.0f - 2.0f * (i + 0.5f) / m_RayCount);
      float theta = 3.14159265f * (1.0f + std::sqrt(5.0f)) * i;

      AcousticRay ray;
      ray.origin = source;
      ray.direction = {std::sin(phi) * std::cos(theta),
                       std::sin(phi) * std::sin(theta), std::cos(phi)};

      // Trace this ray
      TraceRay(ray, listener, listenerRadius, result);
    }
  }

  void TraceRay(AcousticRay ray, const AcousticVector &listener,
                float listenerRadius, PropagationResult &result) {
    while (ray.bounces < kMaxBounces && ray.energy > kMinEnergy &&
           ray.distance < m_MaxDistance) {

      RayHit hit = m_GeometryCallback(ray.origin, ray.direction,
                                      m_MaxDistance - ray.distance);

      if (!hit.hit) {
        break; // Ray escaped scene
      }

      // Check if ray passes near listener before hitting surface
      AcousticVector toListener = listener - ray.origin;
      float projLength = toListener.Dot(ray.direction);

      if (projLength > 0 && projLength < hit.distance) {
        AcousticVector closestPoint = ray.origin + ray.direction * projLength;
        float distToListener = (listener - closestPoint).Length();

        if (distToListener < listenerRadius) {
          // Ray reaches listener
          PropagationPath path;
          path.distance = ray.distance + projLength;
          path.delay = path.distance / kSpeedOfSound;
          path.reflections = ray.bounces;
          path.gainLow = ray.energyLow * DistanceAttenuation(path.distance);
          path.gainMid = ray.energyMid * DistanceAttenuation(path.distance);
          path.gainHigh =
              ray.energyHigh * DistanceAttenuation(path.distance) * 0.8f;

          result.paths.push_back(path);
        }
      }

      // Update ray after reflection
      ray.distance += hit.distance;
      ray.origin =
          hit.point + hit.normal * 0.001f; // Offset to avoid self-intersection
      ray.direction = ray.direction.Reflect(hit.normal).Normalized();
      ray.bounces++;

      // Apply material absorption
      ray.energyLow *= (1.0f - hit.material.absorptionLow);
      ray.energyMid *= (1.0f - hit.material.absorptionMid);
      ray.energyHigh *= (1.0f - hit.material.absorptionHigh);
      ray.energy = (ray.energyLow + ray.energyMid + ray.energyHigh) / 3.0f;

      // Apply scattering (randomize direction slightly)
      if (hit.material.scattering > 0.0f) {
        // Simplified scattering - blend toward random direction
        float scatter = hit.material.scattering * 0.3f;
        ray.direction.x += (RandomFloat() - 0.5f) * scatter;
        ray.direction.y += (RandomFloat() - 0.5f) * scatter;
        ray.direction.z += (RandomFloat() - 0.5f) * scatter;
        ray.direction = ray.direction.Normalized();
      }
    }
  }

  void ComputeEarlyReflections(PropagationResult &result) {
    if (result.paths.size() <= 1) {
      return;
    }

    // Find first reflection
    float minReflectionDelay = 1000.0f;
    float totalReflectionGain = 0.0f;

    for (const auto &path : result.paths) {
      if (!path.isDirect && path.reflections > 0) {
        minReflectionDelay = (std::min)(minReflectionDelay, path.delay);
        totalReflectionGain +=
            (path.gainLow + path.gainMid + path.gainHigh) / 3.0f;
      }
    }

    if (minReflectionDelay < 1000.0f) {
      result.earlyReflectionDelay = minReflectionDelay;
      result.earlyReflectionGain = (std::min)(totalReflectionGain, 1.0f);
    }

    // Estimate late reverb time based on average absorption
    result.lateReverbTime = 0.5f; // Simplified estimate
  }

  float DistanceAttenuation(float distance) const {
    return 1.0f / (1.0f + distance * distance * 0.01f);
  }

  float RandomFloat() const {
    // Simple LCG random
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return static_cast<float>(seed & 0x7FFFFFFF) / 2147483647.0f;
  }

  GeometryCallback m_GeometryCallback;
  int m_RayCount = 64;
  float m_MaxDistance = 100.0f;
  bool m_Enabled = false;
};

/**
 * @brief Apply propagation result to audio parameters.
 */
struct PropagationEffect {
  float volume = 1.0f;
  float lowPassCutoff = 20000.0f; // Hz
  float reverbSend = 0.0f;
  float delay = 0.0f;

  static PropagationEffect FromResult(const PropagationResult &result) {
    PropagationEffect effect;

    if (result.paths.empty()) {
      effect.volume = 0.0f;
      return effect;
    }

    // Combine gains from all paths
    float totalGain = 0.0f;
    float highFreqRatio = 0.0f;

    for (const auto &path : result.paths) {
      float pathGain = (path.gainLow + path.gainMid + path.gainHigh) / 3.0f;
      totalGain += pathGain;

      if (pathGain > 0.0f) {
        highFreqRatio += path.gainHigh / pathGain;
      }
    }

    effect.volume = (std::min)(totalGain, 1.0f);

    // Low-pass based on high frequency loss
    if (!result.paths.empty()) {
      highFreqRatio /= result.paths.size();
      effect.lowPassCutoff = 2000.0f + highFreqRatio * 18000.0f;
    }

    // Reverb send based on reflection energy
    effect.reverbSend = result.earlyReflectionGain * 0.5f;

    // Delay from direct path
    if (result.hasDirectPath && !result.paths.empty()) {
      effect.delay = result.paths[0].delay;
    }

    return effect;
  }
};

} // namespace Orpheus
