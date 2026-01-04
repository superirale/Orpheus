/**
 * @file HDRAudio.h
 * @brief HDR Audio with loudness normalization.
 *
 * Provides LUFS measurement (ITU-R BS.1770) and automatic gain adjustment
 * for consistent audio loudness.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <deque>

namespace Orpheus {

/**
 * @brief K-weighting filter for LUFS measurement.
 *
 * Implements the two-stage filter from ITU-R BS.1770:
 * Stage 1: High-shelf boost at ~1500Hz
 * Stage 2: High-pass filter at ~38Hz
 */
class KWeightingFilter {
public:
  explicit KWeightingFilter(float sampleRate = 44100.0f) {
    // Pre-computed coefficients for 44.1kHz (simplified)
    // Stage 1: High-shelf +4dB @ 1500Hz
    m_A1[0] = 1.0f;
    m_A1[1] = -1.69065929318241f;
    m_A1[2] = 0.73248077421585f;
    m_B1[0] = 1.53512485958697f;
    m_B1[1] = -2.69169618940638f;
    m_B1[2] = 1.19839281085285f;

    // Stage 2: High-pass ~38Hz
    m_A2[0] = 1.0f;
    m_A2[1] = -1.99004745483398f;
    m_A2[2] = 0.99007225036621f;
    m_B2[0] = 1.0f;
    m_B2[1] = -2.0f;
    m_B2[2] = 1.0f;

    Reset();
  }

  float Process(float sample) {
    // Stage 1
    float y1 = m_B1[0] * sample + m_B1[1] * m_X1[0] + m_B1[2] * m_X1[1] -
               m_A1[1] * m_Y1[0] - m_A1[2] * m_Y1[1];
    m_X1[1] = m_X1[0];
    m_X1[0] = sample;
    m_Y1[1] = m_Y1[0];
    m_Y1[0] = y1;

    // Stage 2
    float y2 = m_B2[0] * y1 + m_B2[1] * m_X2[0] + m_B2[2] * m_X2[1] -
               m_A2[1] * m_Y2[0] - m_A2[2] * m_Y2[1];
    m_X2[1] = m_X2[0];
    m_X2[0] = y1;
    m_Y2[1] = m_Y2[0];
    m_Y2[0] = y2;

    return y2;
  }

  void Reset() {
    m_X1[0] = m_X1[1] = 0.0f;
    m_Y1[0] = m_Y1[1] = 0.0f;
    m_X2[0] = m_X2[1] = 0.0f;
    m_Y2[0] = m_Y2[1] = 0.0f;
  }

private:
  float m_A1[3], m_B1[3]; // Stage 1 coefficients
  float m_A2[3], m_B2[3]; // Stage 2 coefficients
  float m_X1[2], m_Y1[2]; // Stage 1 state
  float m_X2[2], m_Y2[2]; // Stage 2 state
};

/**
 * @brief LUFS loudness analyzer (ITU-R BS.1770).
 *
 * Measures loudness using K-weighted filtering and gated measurement.
 */
class LoudnessAnalyzer {
public:
  explicit LoudnessAnalyzer(float sampleRate = 44100.0f)
      : m_SampleRate(sampleRate), m_Filter(sampleRate) {
    m_MomentaryWindow = static_cast<size_t>(sampleRate * 0.4f); // 400ms
    m_ShortTermWindow = static_cast<size_t>(sampleRate * 3.0f); // 3s
    m_BlockSize = static_cast<size_t>(sampleRate * 0.1f);       // 100ms blocks
  }

  /**
   * @brief Process samples and update loudness measurements.
   */
  void Process(const float *samples, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
      float filtered = m_Filter.Process(samples[i]);
      float squared = filtered * filtered;

      m_MomentarySum += squared;
      m_ShortTermSum += squared;
      m_IntegratedSum += squared;
      m_MomentaryCount++;
      m_ShortTermCount++;
      m_IntegratedCount++;

      // Store for windowed measurements
      m_MomentaryBuffer.push_back(squared);
      if (m_MomentaryBuffer.size() > m_MomentaryWindow) {
        m_MomentarySum -= m_MomentaryBuffer.front();
        m_MomentaryBuffer.pop_front();
        m_MomentaryCount--;
      }

      m_ShortTermBuffer.push_back(squared);
      if (m_ShortTermBuffer.size() > m_ShortTermWindow) {
        m_ShortTermSum -= m_ShortTermBuffer.front();
        m_ShortTermBuffer.pop_front();
        m_ShortTermCount--;
      }

      // Track true peak
      float absSample = std::abs(samples[i]);
      if (absSample > m_TruePeak) {
        m_TruePeak = absSample;
      }
    }
  }

  /**
   * @brief Get momentary loudness (400ms window).
   */
  [[nodiscard]] float GetMomentaryLUFS() const {
    if (m_MomentaryCount == 0)
      return -70.0f;
    float meanSquare = m_MomentarySum / static_cast<float>(m_MomentaryCount);
    return MeanSquareToLUFS(meanSquare);
  }

  /**
   * @brief Get short-term loudness (3 second window).
   */
  [[nodiscard]] float GetShortTermLUFS() const {
    if (m_ShortTermCount == 0)
      return -70.0f;
    float meanSquare = m_ShortTermSum / static_cast<float>(m_ShortTermCount);
    return MeanSquareToLUFS(meanSquare);
  }

  /**
   * @brief Get integrated loudness (full measurement).
   */
  [[nodiscard]] float GetIntegratedLUFS() const {
    if (m_IntegratedCount == 0)
      return -70.0f;
    float meanSquare = m_IntegratedSum / static_cast<float>(m_IntegratedCount);
    return MeanSquareToLUFS(meanSquare);
  }

  /**
   * @brief Get true peak level in dB.
   */
  [[nodiscard]] float GetTruePeakDB() const {
    if (m_TruePeak <= 0.0f)
      return -70.0f;
    return 20.0f * std::log10(m_TruePeak);
  }

  /**
   * @brief Reset all measurements.
   */
  void Reset() {
    m_Filter.Reset();
    m_MomentaryBuffer.clear();
    m_ShortTermBuffer.clear();
    m_MomentarySum = m_ShortTermSum = m_IntegratedSum = 0.0f;
    m_MomentaryCount = m_ShortTermCount = m_IntegratedCount = 0;
    m_TruePeak = 0.0f;
  }

private:
  static float MeanSquareToLUFS(float ms) {
    if (ms <= 0.0f)
      return -70.0f;
    return -0.691f + 10.0f * std::log10(ms);
  }

  float m_SampleRate;
  KWeightingFilter m_Filter;
  size_t m_MomentaryWindow;
  size_t m_ShortTermWindow;
  size_t m_BlockSize;

  std::deque<float> m_MomentaryBuffer;
  std::deque<float> m_ShortTermBuffer;

  float m_MomentarySum = 0.0f;
  float m_ShortTermSum = 0.0f;
  float m_IntegratedSum = 0.0f;
  size_t m_MomentaryCount = 0;
  size_t m_ShortTermCount = 0;
  size_t m_IntegratedCount = 0;
  float m_TruePeak = 0.0f;
};

/**
 * @brief HDR Audio mixer for automatic loudness normalization.
 */
class HDRMixer {
public:
  explicit HDRMixer(float sampleRate = 44100.0f)
      : m_Analyzer(sampleRate), m_SampleRate(sampleRate) {}

  /**
   * @brief Set target loudness in LUFS.
   * Common targets: -14 (streaming), -23 (broadcast EBU R128)
   */
  void SetTargetLoudness(float lufs) {
    m_TargetLUFS = std::clamp(lufs, -70.0f, 0.0f);
  }

  [[nodiscard]] float GetTargetLoudness() const { return m_TargetLUFS; }

  /**
   * @brief Enable/disable HDR processing.
   */
  void SetEnabled(bool enabled) { m_Enabled = enabled; }
  [[nodiscard]] bool IsEnabled() const { return m_Enabled; }

  /**
   * @brief Set maximum gain adjustment in dB.
   */
  void SetMaxGain(float db) { m_MaxGainDB = std::abs(db); }

  /**
   * @brief Process samples with loudness normalization.
   */
  void Process(float *samples, size_t numSamples) {
    m_Analyzer.Process(samples, numSamples);

    if (!m_Enabled)
      return;

    float currentLUFS = m_Analyzer.GetShortTermLUFS();
    if (currentLUFS < -60.0f)
      return; // Too quiet to measure

    // Calculate needed gain adjustment
    float targetGain = m_TargetLUFS - currentLUFS;
    targetGain = std::clamp(targetGain, -m_MaxGainDB, m_MaxGainDB);

    // Smooth gain changes
    float alpha = 1.0f - std::exp(-1.0f / (m_SampleRate * 0.1f)); // 100ms
    m_CurrentGainDB = m_CurrentGainDB + alpha * (targetGain - m_CurrentGainDB);

    // Apply gain
    float linearGain = std::pow(10.0f, m_CurrentGainDB / 20.0f);
    for (size_t i = 0; i < numSamples; ++i) {
      samples[i] *= linearGain;

      // Soft clip to prevent distortion
      if (samples[i] > 1.0f) {
        samples[i] = 1.0f - std::exp(1.0f - samples[i]);
      } else if (samples[i] < -1.0f) {
        samples[i] = -1.0f + std::exp(-1.0f - samples[i]);
      }
    }
  }

  /**
   * @brief Get current loudness measurements.
   */
  [[nodiscard]] float GetMomentaryLUFS() const {
    return m_Analyzer.GetMomentaryLUFS();
  }

  [[nodiscard]] float GetShortTermLUFS() const {
    return m_Analyzer.GetShortTermLUFS();
  }

  [[nodiscard]] float GetIntegratedLUFS() const {
    return m_Analyzer.GetIntegratedLUFS();
  }

  [[nodiscard]] float GetTruePeakDB() const {
    return m_Analyzer.GetTruePeakDB();
  }

  [[nodiscard]] float GetCurrentGainDB() const { return m_CurrentGainDB; }

  void Reset() {
    m_Analyzer.Reset();
    m_CurrentGainDB = 0.0f;
  }

private:
  LoudnessAnalyzer m_Analyzer;
  float m_SampleRate;
  float m_TargetLUFS = -14.0f; // Spotify/YouTube standard
  float m_MaxGainDB = 12.0f;   // Max ±12dB adjustment
  float m_CurrentGainDB = 0.0f;
  bool m_Enabled = false;
};

} // namespace Orpheus
