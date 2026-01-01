/**
 * @file OcclusionQuery.h
 * @brief Occlusion query interface for game engine integration.
 *
 * Defines the callback type for occlusion raycasts that the game
 * engine must provide.
 */
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Types.h"

namespace Orpheus {

/**
 * @brief Result of a single raycast hit against geometry.
 *
 * Returned by the occlusion query callback to describe what
 * materials were hit between sound source and listener.
 */
struct OcclusionHit {
  std::string
      materialName;       ///< Name of material hit (matches OcclusionMaterial)
  float thickness = 1.0f; ///< Estimated thickness in world units

  /**
   * @brief Default constructor.
   */
  OcclusionHit() = default;

  /**
   * @brief Construct a hit result.
   * @param mat Material name.
   * @param thick Thickness in world units (default: 1.0).
   */
  OcclusionHit(const std::string &mat, float thick = 1.0f)
      : materialName(mat), thickness(thick) {}
};

/**
 * @brief Callback type for occlusion queries.
 *
 * The game/engine provides this callback to perform raycasts between
 * sound sources and the listener. The audio engine calls this to
 * determine what materials are blocking each sound.
 *
 * @param source Position of the sound source.
 * @param listener Position of the listener.
 * @return Vector of hits encountered along the ray from source to listener.
 *         Empty vector means unobstructed line of sight.
 *
 * @par Example Implementation:
 * @code
 * audio.SetOcclusionQueryCallback([&](const Vector3& src, const Vector3& dst) {
 *     std::vector<OcclusionHit> hits;
 *     for (auto& hit : physics.raycast(src, dst)) {
 *         hits.push_back({hit.materialName, hit.thickness});
 *     }
 *     return hits;
 * });
 * @endcode
 */
using OcclusionQueryCallback = std::function<std::vector<OcclusionHit>(
    const Vector3 &source, const Vector3 &listener)>;

} // namespace Orpheus
