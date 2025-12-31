#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Types.h"

namespace Orpheus {

// Result of a single raycast hit against geometry
struct OcclusionHit {
  std::string materialName; // Name of material hit (matches OcclusionMaterial)
  float thickness = 1.0f;   // Estimated thickness in world units

  OcclusionHit() = default;
  OcclusionHit(const std::string &mat, float thick = 1.0f)
      : materialName(mat), thickness(thick) {}
};

// Callback type for occlusion queries.
// The game/engine provides this callback to perform raycasts.
//
// Parameters:
//   source   - Position of the sound source
//   listener - Position of the listener
//
// Returns:
//   Vector of hits encountered along the ray from source to listener.
//   Empty vector means unobstructed line of sight.
//
// Example implementation:
//   audio.SetOcclusionQueryCallback([&](const Vector3& src, const Vector3& dst)
//   {
//     std::vector<OcclusionHit> hits;
//     for (auto& hit : physics.raycast(src, dst)) {
//       hits.push_back({hit.materialName, hit.thickness});
//     }
//     return hits;
//   });
using OcclusionQueryCallback = std::function<std::vector<OcclusionHit>(
    const Vector3 &source, const Vector3 &listener)>;

} // namespace Orpheus
