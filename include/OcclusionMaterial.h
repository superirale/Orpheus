// OcclusionMaterial.h
#pragma once

// Defines how a material affects sound propagation
struct OcclusionMaterial {
  std::string name;
  float obstruction = 0.5f;   // 0.0-1.0 how much it blocks (partial)
  float occlusionBias = 0.0f; // Additional bias towards full occlusion

  OcclusionMaterial() = default;
  OcclusionMaterial(const std::string &n, float obs, float bias = 0.0f)
      : name(n), obstruction(obs), occlusionBias(bias) {}
};

// Built-in material presets
namespace OcclusionMaterials {
// Thin/light materials - low obstruction
inline const OcclusionMaterial Glass{"Glass", 0.2f, -0.2f};
inline const OcclusionMaterial Fabric{"Fabric", 0.1f, -0.3f};
inline const OcclusionMaterial Foliage{"Foliage", 0.15f, -0.2f};

// Medium materials
inline const OcclusionMaterial Wood{"Wood", 0.3f, 0.0f};
inline const OcclusionMaterial Plaster{"Plaster", 0.4f, 0.1f};
inline const OcclusionMaterial Metal{"Metal", 0.5f, 0.1f};

// Heavy materials - high obstruction
inline const OcclusionMaterial Brick{"Brick", 0.6f, 0.2f};
inline const OcclusionMaterial Concrete{"Concrete", 0.8f, 0.3f};
inline const OcclusionMaterial Stone{"Stone", 0.85f, 0.35f};

// Full blockers
inline const OcclusionMaterial Terrain{"Terrain", 1.0f, 0.5f};
inline const OcclusionMaterial Water{"Water", 0.9f, 0.4f};

// Default fallback
inline const OcclusionMaterial Default{"Default", 0.5f, 0.0f};
} // namespace OcclusionMaterials
