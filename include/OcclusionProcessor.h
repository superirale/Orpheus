/**
 * @file OcclusionProcessor.h
 * @brief Processes audio occlusion for realistic sound propagation.
 *
 * Handles occlusion calculations and applies DSP effects (lowpass
 * filter, volume reduction) based on obstacles between sounds and listener.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

#include "OcclusionMaterial.h"
#include "OcclusionQuery.h"
#include "OpaqueHandles.h"
#include "Voice.h"

namespace Orpheus {

/**
 * @brief Processes occlusion for voices and applies DSP effects.
 *
 * OcclusionProcessor queries the game engine for obstacles between
 * each sound source and the listener, then applies appropriate
 * filtering and volume reduction to simulate sound propagation
 * through materials.
 *
 * @par Features:
 * - Configurable update rate for performance tuning
 * - Smooth value interpolation to avoid audio artifacts
 * - Lowpass filtering and volume reduction based on material properties
 * - Support for custom material definitions
 */
class OcclusionProcessor {
public:
  /**
   * @brief Default constructor.
   */
  OcclusionProcessor();

  /**
   * @brief Set the callback for occlusion queries.
   *
   * The game engine must provide this callback to perform raycasts.
   * @param callback Function that performs raycasts between positions.
   */
  void SetQueryCallback(OcclusionQueryCallback callback);

  /**
   * @brief Register a custom occlusion material.
   * @param mat The material to register.
   */
  void RegisterMaterial(const OcclusionMaterial &mat);

  /// @name Configuration
  /// @{

  /**
   * @brief Set the occlusion threshold.
   *
   * Occlusion values above this trigger full occlusion effects.
   * @param threshold Threshold value (0.0-1.0, default: 0.7).
   */
  void SetOcclusionThreshold(float threshold);

  /**
   * @brief Set smoothing time for occlusion transitions.
   * @param seconds Interpolation time (default: 0.1s).
   */
  void SetSmoothingTime(float seconds);

  /**
   * @brief Set the occlusion update rate.
   *
   * Lower rates improve performance but reduce accuracy.
   * @param hz Updates per second (default: 10 Hz).
   */
  void SetUpdateRate(float hz);

  /**
   * @brief Enable or disable occlusion processing.
   * @param enabled true to enable.
   */
  void SetEnabled(bool enabled);

  /// @}

  /// @name DSP Configuration
  /// @{

  /**
   * @brief Set the lowpass filter frequency range.
   * @param minFreq Minimum frequency at full occlusion (Hz).
   * @param maxFreq Maximum frequency at no occlusion (Hz).
   */
  void SetLowPassRange(float minFreq, float maxFreq);

  /**
   * @brief Set maximum volume reduction from occlusion.
   * @param maxReduction Maximum reduction (0.0-1.0).
   */
  void SetVolumeReduction(float maxReduction);

  /// @}

  /**
   * @brief Update occlusion for a voice.
   *
   * Queries occlusion and updates the voice's occlusion state.
   * @param voice The voice to update.
   * @param listenerPos Current listener position.
   * @param dt Delta time in seconds.
   */
  void Update(Voice &voice, const Vector3 &listenerPos, float dt);

  /**
   * @brief Apply DSP effects to a playing voice.
   *
   * Sets filter and volume based on calculated occlusion.
   * @param engine Native engine handle.
   * @param voice The voice to apply effects to.
   */
  void ApplyDSP(NativeEngineHandle engine, Voice &voice);

  /// @name State Queries
  /// @{

  /**
   * @brief Check if occlusion is enabled.
   * @return true if enabled.
   */
  [[nodiscard]] bool IsEnabled() const;

  /**
   * @brief Get the occlusion threshold.
   * @return Current threshold value.
   */
  [[nodiscard]] float GetOcclusionThreshold() const;

  /// @}

private:
  void RegisterDefaultMaterials();
  const OcclusionMaterial &GetMaterial(const std::string &name) const;
  void SmoothValues(Voice &voice, float dt);

  OcclusionQueryCallback m_QueryCallback;
  std::unordered_map<std::string, OcclusionMaterial> m_Materials;

  bool m_Enabled = true;
  float m_OcclusionThreshold = 0.7f;
  float m_SmoothingTime = 0.1f;
  float m_UpdateRate = 10.0f;

  float m_MinLowPassFreq = 400.0f;
  float m_MaxLowPassFreq = 22000.0f;
  float m_MaxVolumeReduction = 0.5f;

  float m_TimeSinceLastUpdate = 0.0f;
};

} // namespace Orpheus
