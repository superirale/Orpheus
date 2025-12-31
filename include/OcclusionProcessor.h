#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

#include <soloud.h>
#include <soloud_biquadresonantfilter.h>

#include "OcclusionMaterial.h"
#include "OcclusionQuery.h"
#include "Voice.h"

namespace Orpheus {

// Processes occlusion for voices and applies DSP effects.
class OcclusionProcessor {
public:
  OcclusionProcessor();

  // Set the callback for occlusion queries (game provides raycasts)
  void SetQueryCallback(OcclusionQueryCallback callback);

  // Register a custom material
  void RegisterMaterial(const OcclusionMaterial &mat);

  // Configuration
  void SetOcclusionThreshold(float threshold);
  void SetSmoothingTime(float seconds);
  void SetUpdateRate(float hz);
  void SetEnabled(bool enabled);

  // DSP range configuration
  void SetLowPassRange(float minFreq, float maxFreq);
  void SetVolumeReduction(float maxReduction);

  // Update occlusion for a voice (called each audio update)
  void Update(Voice &voice, const Vector3 &listenerPos, float dt);

  // Apply DSP effects to a playing voice
  void ApplyDSP(SoLoud::Soloud &engine, Voice &voice);

  // Get current state for debugging
  bool IsEnabled() const;
  float GetOcclusionThreshold() const;

private:
  void RegisterDefaultMaterials();
  const OcclusionMaterial &GetMaterial(const std::string &name) const;
  void SmoothValues(Voice &voice, float dt);

  // Callback for occlusion queries
  OcclusionQueryCallback m_QueryCallback;

  // Registered materials
  std::unordered_map<std::string, OcclusionMaterial> m_Materials;

  // Configuration
  bool m_Enabled = true;
  float m_OcclusionThreshold = 0.7f;
  float m_SmoothingTime = 0.1f;
  float m_UpdateRate = 10.0f;

  // DSP mapping
  float m_MinLowPassFreq = 400.0f;
  float m_MaxLowPassFreq = 22000.0f;
  float m_MaxVolumeReduction = 0.5f;

  // Update timing
  float m_TimeSinceLastUpdate = 0.0f;
};

} // namespace Orpheus
