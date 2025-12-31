// OcclusionProcessor.h
#pragma once

#include "OcclusionMaterial.h"
#include "OcclusionQuery.h"
#include "Voice.h"

// Processes occlusion for voices and applies DSP effects.
// The game provides raycast results via a callback; this class
// calculates obstruction/occlusion values and maps them to DSP parameters.
class OcclusionProcessor {
public:
  OcclusionProcessor() { RegisterDefaultMaterials(); }

  // Set the callback for occlusion queries (game provides raycasts)
  void SetQueryCallback(OcclusionQueryCallback callback) {
    m_QueryCallback = std::move(callback);
  }

  // Register a custom material
  void RegisterMaterial(const OcclusionMaterial &mat) {
    m_Materials[mat.name] = mat;
  }

  // Configuration
  void SetOcclusionThreshold(float threshold) {
    m_OcclusionThreshold = std::clamp(threshold, 0.0f, 1.0f);
  }
  void SetSmoothingTime(float seconds) {
    m_SmoothingTime = std::max(seconds, 0.01f);
  }
  void SetUpdateRate(float hz) { m_UpdateRate = std::max(hz, 1.0f); }
  void SetEnabled(bool enabled) { m_Enabled = enabled; }

  // DSP range configuration
  void SetLowPassRange(float minFreq, float maxFreq) {
    m_MinLowPassFreq = std::clamp(minFreq, 100.0f, 22000.0f);
    m_MaxLowPassFreq = std::clamp(maxFreq, m_MinLowPassFreq, 22000.0f);
  }
  void SetVolumeReduction(float maxReduction) {
    m_MaxVolumeReduction = std::clamp(maxReduction, 0.0f, 1.0f);
  }

  // Update occlusion for a voice (called each audio update)
  void Update(Voice &voice, const Vector3 &listenerPos, float dt) {
    if (!m_Enabled || !m_QueryCallback) {
      // Reset to unoccluded
      voice.obstruction = 0.0f;
      voice.occlusion = 0.0f;
      voice.targetLowPassFreq = m_MaxLowPassFreq;
      voice.occlusionVolume = 1.0f;
      SmoothValues(voice, dt);
      return;
    }

    // Rate limiting: only query at configured rate
    m_TimeSinceLastUpdate += dt;
    if (m_TimeSinceLastUpdate < 1.0f / m_UpdateRate) {
      // Just smooth existing values
      SmoothValues(voice, dt);
      return;
    }
    m_TimeSinceLastUpdate = 0.0f;

    // Query for occlusion hits
    auto hits = m_QueryCallback(voice.position, listenerPos);

    // Calculate obstruction from hits
    float totalObstruction = 0.0f;
    float totalOcclusionBias = 0.0f;

    for (const auto &hit : hits) {
      const OcclusionMaterial &mat = GetMaterial(hit.materialName);
      // Scale by thickness (thicker = more obstruction)
      float thicknessFactor = std::min(hit.thickness, 3.0f) / 3.0f;
      totalObstruction += mat.obstruction * (0.5f + 0.5f * thicknessFactor);
      totalOcclusionBias += mat.occlusionBias;
    }

    // Clamp obstruction to 0-1
    voice.obstruction = std::clamp(totalObstruction, 0.0f, 1.0f);

    // Calculate occlusion (binary-ish, with threshold)
    float occlusionValue = voice.obstruction + totalOcclusionBias;
    if (occlusionValue >= m_OcclusionThreshold) {
      // Full occlusion - ramp up from threshold to 1.0
      float t = (occlusionValue - m_OcclusionThreshold) /
                (1.0f - m_OcclusionThreshold);
      voice.occlusion = std::clamp(t, 0.0f, 1.0f);
    } else {
      voice.occlusion = 0.0f;
    }

    // Calculate target DSP parameters
    float combined = std::max(voice.obstruction, voice.occlusion);

    // Low-pass frequency: exponential mapping for more natural feel
    // 0.0 -> maxFreq, 1.0 -> minFreq
    float freqT = 1.0f - combined;
    voice.targetLowPassFreq =
        m_MinLowPassFreq * std::pow(m_MaxLowPassFreq / m_MinLowPassFreq, freqT);

    // Volume reduction: linear mapping
    // 0.0 -> 1.0 volume, 1.0 -> (1-maxReduction) volume
    voice.occlusionVolume = 1.0f - (combined * m_MaxVolumeReduction);

    // Smooth values
    SmoothValues(voice, dt);
  }

  // Apply DSP effects to a playing voice
  void ApplyDSP(SoLoud::Soloud &engine, Voice &voice) {
    if (!m_Enabled || voice.handle == 0)
      return;

    // Apply volume modifier (combines base volume with occlusion reduction)
    float occludedVolume = voice.volume * voice.occlusionVolume;
    engine.setVolume(voice.handle, occludedVolume);

    // Always set low-pass filter frequency (even when unoccluded = 22kHz)
    // This ensures smooth transitions between occluded and unoccluded states
    engine.setFilterParameter(voice.handle, 0,
                              SoLoud::BiquadResonantFilter::FREQUENCY,
                              voice.currentLowPassFreq);
  }

  // Get current state for debugging
  bool IsEnabled() const { return m_Enabled; }
  float GetOcclusionThreshold() const { return m_OcclusionThreshold; }

private:
  void RegisterDefaultMaterials() {
    RegisterMaterial(OcclusionMaterials::Glass);
    RegisterMaterial(OcclusionMaterials::Fabric);
    RegisterMaterial(OcclusionMaterials::Foliage);
    RegisterMaterial(OcclusionMaterials::Wood);
    RegisterMaterial(OcclusionMaterials::Plaster);
    RegisterMaterial(OcclusionMaterials::Metal);
    RegisterMaterial(OcclusionMaterials::Brick);
    RegisterMaterial(OcclusionMaterials::Concrete);
    RegisterMaterial(OcclusionMaterials::Stone);
    RegisterMaterial(OcclusionMaterials::Terrain);
    RegisterMaterial(OcclusionMaterials::Water);
    RegisterMaterial(OcclusionMaterials::Default);
  }

  const OcclusionMaterial &GetMaterial(const std::string &name) const {
    auto it = m_Materials.find(name);
    if (it != m_Materials.end()) {
      return it->second;
    }
    // Return default material
    static const OcclusionMaterial defaultMat = OcclusionMaterials::Default;
    return defaultMat;
  }

  void SmoothValues(Voice &voice, float dt) {
    // Exponential smoothing for low-pass frequency
    float alpha = 1.0f - std::exp(-dt / m_SmoothingTime);
    voice.currentLowPassFreq +=
        alpha * (voice.targetLowPassFreq - voice.currentLowPassFreq);
    voice.occlusionSmoothed +=
        alpha * (voice.occlusion - voice.occlusionSmoothed);
  }

  // Callback for occlusion queries
  OcclusionQueryCallback m_QueryCallback;

  // Registered materials
  std::unordered_map<std::string, OcclusionMaterial> m_Materials;

  // Configuration
  bool m_Enabled = true;
  float m_OcclusionThreshold = 0.7f; // When obstruction becomes occlusion
  float m_SmoothingTime = 0.1f;      // 100ms smoothing
  float m_UpdateRate = 10.0f;        // 10 Hz update rate

  // DSP mapping
  float m_MinLowPassFreq = 400.0f;   // Fully occluded
  float m_MaxLowPassFreq = 22000.0f; // Unoccluded
  float m_MaxVolumeReduction = 0.5f; // Max 50% volume reduction

  // Update timing
  float m_TimeSinceLastUpdate = 0.0f;
};
