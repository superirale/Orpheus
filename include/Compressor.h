/**
 * @file Compressor.h
 * @brief Dynamic range compression for audio buses.
 *
 * Provides compressor and limiter functionality for controlling audio
 * dynamics.
 */
#pragma once

#include <algorithm>
#include <cmath>

namespace Orpheus {

/**
 * @brief Settings for a compressor/limiter.
 */
struct CompressorSettings {
  float threshold = -10.0f; ///< Compression threshold in dB
  float ratio = 4.0f;       ///< Compression ratio (e.g., 4:1)
  float attackMs = 10.0f;   ///< Attack time in milliseconds
  float releaseMs = 100.0f; ///< Release time in milliseconds
  float makeupGain = 0.0f;  ///< Makeup gain in dB
  bool limiterMode = false; ///< true = hard limiter (infinite ratio)
};

/**
 * @brief Compressor/Limiter for dynamic range control.
 *
 * Reduces the dynamic range of audio by attenuating signals that
 * exceed a threshold. Can be configured as a soft compressor or
 * hard limiter.
 *
 * @par Example Usage:
 * @code
 * CompressorSettings settings;
 * settings.threshold = -12.0f;
 * settings.ratio = 4.0f;
 * settings.attackMs = 5.0f;
 * settings.releaseMs = 50.0f;
 *
 * Compressor comp(44100);
 * comp.SetSettings(settings);
 * comp.Process(samples, numSamples);
 * @endcode
 */
class Compressor {
public:
  /**
   * @brief Create a compressor with given sample rate.
   * @param sampleRate Audio sample rate (e.g., 44100).
   */
  explicit Compressor(float sampleRate = 44100.0f) : m_SampleRate(sampleRate) {
    UpdateCoefficients();
  }

  /**
   * @brief Set compressor settings.
   * @param settings Compressor configuration.
   */
  void SetSettings(const CompressorSettings &settings) {
    m_Settings = settings;
    UpdateCoefficients();
  }

  /**
   * @brief Get current settings.
   */
  [[nodiscard]] const CompressorSettings &GetSettings() const {
    return m_Settings;
  }

  /**
   * @brief Enable/disable the compressor.
   */
  void SetEnabled(bool enabled) { m_Enabled = enabled; }

  /**
   * @brief Check if compressor is enabled.
   */
  [[nodiscard]] bool IsEnabled() const { return m_Enabled; }

  /**
   * @brief Process audio samples in-place.
   * @param samples Pointer to audio sample buffer.
   * @param numSamples Number of samples to process.
   */
  void Process(float *samples, size_t numSamples) {
    if (!m_Enabled || samples == nullptr || numSamples == 0) {
      return;
    }

    for (size_t i = 0; i < numSamples; ++i) {
      float input = samples[i];
      float inputLevel = std::abs(input);

      // Convert to dB
      float inputDb = LinearToDb(inputLevel);

      // Calculate gain reduction
      float gainReductionDb = 0.0f;
      if (inputDb > m_Settings.threshold) {
        if (m_Settings.limiterMode) {
          // Hard limiter: reduce everything above threshold
          gainReductionDb = m_Settings.threshold - inputDb;
        } else {
          // Compressor: apply ratio
          float excess = inputDb - m_Settings.threshold;
          float compressed = excess / m_Settings.ratio;
          gainReductionDb = compressed - excess;
        }
      }

      // Apply envelope (attack/release smoothing)
      if (gainReductionDb < m_Envelope) {
        // Attack (getting louder, need more reduction)
        m_Envelope =
            m_AttackCoeff * (m_Envelope - gainReductionDb) + gainReductionDb;
      } else {
        // Release (getting quieter, less reduction needed)
        m_Envelope =
            m_ReleaseCoeff * (m_Envelope - gainReductionDb) + gainReductionDb;
      }

      // Apply gain reduction + makeup gain
      float outputGain = DbToLinear(m_Envelope + m_Settings.makeupGain);
      samples[i] = input * outputGain;
    }
  }

  /**
   * @brief Get the current gain reduction in dB.
   */
  [[nodiscard]] float GetGainReduction() const { return -m_Envelope; }

  /**
   * @brief Reset the compressor state.
   */
  void Reset() { m_Envelope = 0.0f; }

private:
  void UpdateCoefficients() {
    // Convert ms to samples, then to coefficient
    float attackSamples = (m_Settings.attackMs / 1000.0f) * m_SampleRate;
    float releaseSamples = (m_Settings.releaseMs / 1000.0f) * m_SampleRate;

    m_AttackCoeff = std::exp(-1.0f / (std::max)(1.0f, attackSamples));
    m_ReleaseCoeff = std::exp(-1.0f / (std::max)(1.0f, releaseSamples));
  }

  static float LinearToDb(float linear) {
    const float minDb = -96.0f;
    if (linear <= 0.0f) {
      return minDb;
    }
    return (std::max)(minDb, 20.0f * std::log10(linear));
  }

  static float DbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

  CompressorSettings m_Settings;
  float m_SampleRate;
  float m_AttackCoeff = 0.0f;
  float m_ReleaseCoeff = 0.0f;
  float m_Envelope = 0.0f;
  bool m_Enabled = false;
};

} // namespace Orpheus
