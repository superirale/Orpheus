/**
 * @file SurroundAudio.h
 * @brief 5.1/7.1 surround sound support with VBAP panning and LFE routing.
 *
 * Provides multi-speaker support including:
 * - Speaker layout detection
 * - Vector-Based Amplitude Panning (VBAP)
 * - Explicit LFE (subwoofer) routing
 * - Downmixing utilities
 */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace Orpheus {

/**
 * @brief Supported speaker layouts.
 */
enum class SpeakerLayout {
  Mono = 1,
  Stereo = 2,
  Quad = 4,
  Surround51 = 6,
  Surround71 = 8
};

/**
 * @brief Speaker channel indices for 5.1 layout.
 */
enum class Channel51 {
  FrontLeft = 0,
  FrontRight = 1,
  Center = 2,
  LFE = 3,
  SurroundLeft = 4,
  SurroundRight = 5
};

/**
 * @brief Speaker channel indices for 7.1 layout.
 */
enum class Channel71 {
  FrontLeft = 0,
  FrontRight = 1,
  Center = 2,
  LFE = 3,
  SurroundLeft = 4,
  SurroundRight = 5,
  BackLeft = 6,
  BackRight = 7
};

/**
 * @brief Speaker gains for a single voice across all channels.
 */
struct SpeakerGains {
  std::array<float, 8> gains = {0.0f};

  void Reset() { gains.fill(0.0f); }
};

/**
 * @brief VBAP-based surround panner for 5.1/7.1 layouts.
 *
 * Calculates per-speaker gains based on 3D position using
 * Vector-Based Amplitude Panning for accurate localization.
 */
class SurroundPanner {
public:
  /**
   * @brief Calculate speaker gains for a 3D position.
   * @param x Left(-1) to Right(+1)
   * @param y Front(-1) to Back(+1)
   * @param z Up(+1) to Down(-1) (affects center/LFE blend)
   * @param layout Target speaker layout
   * @return Speaker gains for all channels
   */
  static SpeakerGains CalculateGains(float x, float y, float z,
                                     SpeakerLayout layout) {
    SpeakerGains gains;

    // Clamp inputs
    x = std::clamp(x, -1.0f, 1.0f);
    y = std::clamp(y, -1.0f, 1.0f);
    z = std::clamp(z, -1.0f, 1.0f);

    switch (layout) {
    case SpeakerLayout::Mono:
      gains.gains[0] = 1.0f;
      break;

    case SpeakerLayout::Stereo:
      CalculateStereo(x, gains);
      break;

    case SpeakerLayout::Quad:
      CalculateQuad(x, y, gains);
      break;

    case SpeakerLayout::Surround51:
      Calculate51(x, y, z, gains);
      break;

    case SpeakerLayout::Surround71:
      Calculate71(x, y, z, gains);
      break;
    }

    // Normalize to preserve loudness
    NormalizeGains(gains, static_cast<int>(layout));

    return gains;
  }

  /**
   * @brief Apply center channel bias for dialogue/UI sounds.
   * @param gains Existing speaker gains
   * @param centerBias 0.0 (no bias) to 1.0 (full center)
   */
  static void ApplyCenterBias(SpeakerGains &gains, float centerBias) {
    centerBias = std::clamp(centerBias, 0.0f, 1.0f);
    if (centerBias <= 0.0f) {
      return;
    }

    // Blend front channels toward center
    float leftRight = (gains.gains[0] + gains.gains[1]) * 0.5f;
    gains.gains[2] += leftRight * centerBias;
    gains.gains[0] *= (1.0f - centerBias * 0.5f);
    gains.gains[1] *= (1.0f - centerBias * 0.5f);
  }

private:
  static void CalculateStereo(float x, SpeakerGains &gains) {
    // Simple equal-power panning
    float pan = (x + 1.0f) * 0.5f;          // 0 to 1
    gains.gains[0] = std::sqrt(1.0f - pan); // Left
    gains.gains[1] = std::sqrt(pan);        // Right
  }

  static void CalculateQuad(float x, float y, SpeakerGains &gains) {
    float frontBack = (y + 1.0f) * 0.5f; // 0 (front) to 1 (back)
    float leftRight = (x + 1.0f) * 0.5f; // 0 (left) to 1 (right)

    float front = 1.0f - frontBack;
    float back = frontBack;
    float left = 1.0f - leftRight;
    float right = leftRight;

    gains.gains[0] = std::sqrt(front * left);  // Front Left
    gains.gains[1] = std::sqrt(front * right); // Front Right
    gains.gains[2] = std::sqrt(back * left);   // Back Left
    gains.gains[3] = std::sqrt(back * right);  // Back Right
  }

  static void Calculate51(float x, float y, float z, SpeakerGains &gains) {
    float frontBack = std::clamp((y + 1.0f) * 0.5f, 0.0f, 1.0f);
    float leftRight = std::clamp((x + 1.0f) * 0.5f, 0.0f, 1.0f);

    float front = 1.0f - frontBack;
    float back = frontBack;
    float left = 1.0f - leftRight;
    float right = leftRight;

    // Front speakers
    gains.gains[static_cast<int>(Channel51::FrontLeft)] =
        std::sqrt(front * left);
    gains.gains[static_cast<int>(Channel51::FrontRight)] =
        std::sqrt(front * right);

    // Center gets contribution from front-center positions
    float centerWeight = front * (1.0f - std::abs(x));
    gains.gains[static_cast<int>(Channel51::Center)] =
        std::sqrt(centerWeight) * 0.7f;

    // Surround speakers
    gains.gains[static_cast<int>(Channel51::SurroundLeft)] =
        std::sqrt(back * left);
    gains.gains[static_cast<int>(Channel51::SurroundRight)] =
        std::sqrt(back * right);

    // LFE is explicitly controlled, not calculated from position
    // (LFE gain set separately via SetLFEGain)
    gains.gains[static_cast<int>(Channel51::LFE)] = 0.0f;
  }

  static void Calculate71(float x, float y, float z, SpeakerGains &gains) {
    // Start with 5.1 calculation
    Calculate51(x, y, z, gains);

    // Add side/back separation for 7.1
    float frontBack = std::clamp((y + 1.0f) * 0.5f, 0.0f, 1.0f);
    float leftRight = std::clamp((x + 1.0f) * 0.5f, 0.0f, 1.0f);

    // Split rear between surround (side) and back channels
    float sideWeight = (std::max)(0.0f, 1.0f - std::abs(y));
    float backWeight = (std::max)(0.0f, frontBack - 0.5f) * 2.0f;

    float left = 1.0f - leftRight;
    float right = leftRight;

    // Redistribute surround to side/back
    float surroundL = gains.gains[static_cast<int>(Channel51::SurroundLeft)];
    float surroundR = gains.gains[static_cast<int>(Channel51::SurroundRight)];

    gains.gains[static_cast<int>(Channel51::SurroundLeft)] =
        surroundL * sideWeight;
    gains.gains[static_cast<int>(Channel51::SurroundRight)] =
        surroundR * sideWeight;
    gains.gains[static_cast<int>(Channel71::BackLeft)] =
        surroundL * backWeight * left;
    gains.gains[static_cast<int>(Channel71::BackRight)] =
        surroundR * backWeight * right;
  }

  static void NormalizeGains(SpeakerGains &gains, int numChannels) {
    float sum = 0.0f;
    for (int i = 0; i < numChannels; ++i) {
      sum += gains.gains[i] * gains.gains[i];
    }
    if (sum > 0.0f) {
      float scale = 1.0f / std::sqrt(sum);
      for (int i = 0; i < numChannels; ++i) {
        gains.gains[i] *= scale;
      }
    }
  }
};

/**
 * @brief LFE routing with explicit control.
 *
 * LFE is INTENTIONAL, never automatic. Low-frequency energy
 * must be explicitly routed to the subwoofer channel.
 */
class LFERouter {
public:
  /**
   * @brief Calculate LFE gain with all modifiers.
   * @param soundLFE Base LFE gain for this sound (0-1)
   * @param snapshotLFE Snapshot LFE multiplier (0-1)
   * @param busLFE Bus LFE multiplier (0-1)
   * @param distance Distance from listener
   * @param lfeRange Maximum LFE range
   * @param occlusionFactor Occlusion (low freq propagates through walls)
   * @return Final LFE gain
   */
  static float CalculateLFE(float soundLFE, float snapshotLFE, float busLFE,
                            float distance, float lfeRange,
                            float occlusionFactor = 1.0f) {
    // Distance attenuation
    float distanceFactor = 1.0f;
    if (lfeRange > 0.0f) {
      distanceFactor = std::clamp(1.0f - (distance / lfeRange), 0.0f, 1.0f);
    }

    // LFE propagates through geometry better than high frequencies
    float effectiveOcclusion = 0.7f + 0.3f * occlusionFactor;

    // Final LFE calculation
    float lfe =
        soundLFE * snapshotLFE * busLFE * distanceFactor * effectiveOcclusion;

    return std::clamp(lfe, 0.0f, 1.0f);
  }

  /**
   * @brief Apply LFE gain to speaker gains.
   * @param gains Speaker gains to modify
   * @param lfeGain LFE level (0-1)
   * @param layout Target speaker layout
   */
  static void ApplyLFE(SpeakerGains &gains, float lfeGain,
                       SpeakerLayout layout) {
    if (layout == SpeakerLayout::Surround51 ||
        layout == SpeakerLayout::Surround71) {
      gains.gains[static_cast<int>(Channel51::LFE)] =
          std::clamp(lfeGain, 0.0f, 1.0f);
    }
  }
};

/**
 * @brief Downmix utilities for speaker layout conversion.
 */
class Downmixer {
public:
  /**
   * @brief Downmix 5.1 gains to stereo.
   */
  static SpeakerGains Downmix51ToStereo(const SpeakerGains &gains51) {
    SpeakerGains stereo;

    // ITU-R BS.775 downmix coefficients
    constexpr float kCenter = 0.707f;   // -3dB
    constexpr float kSurround = 0.707f; // -3dB
    constexpr float kLFE = 0.0f;        // LFE typically not included in stereo

    stereo.gains[0] = gains51.gains[0] + kCenter * gains51.gains[2] +
                      kSurround * gains51.gains[4] + kLFE * gains51.gains[3];

    stereo.gains[1] = gains51.gains[1] + kCenter * gains51.gains[2] +
                      kSurround * gains51.gains[5] + kLFE * gains51.gains[3];

    // Normalize to prevent clipping
    float maxGain = (std::max)(stereo.gains[0], stereo.gains[1]);
    if (maxGain > 1.0f) {
      stereo.gains[0] /= maxGain;
      stereo.gains[1] /= maxGain;
    }

    return stereo;
  }

  /**
   * @brief Downmix 7.1 gains to 5.1.
   */
  static SpeakerGains Downmix71To51(const SpeakerGains &gains71) {
    SpeakerGains gains51;

    // Copy front channels directly
    for (int i = 0; i < 6; ++i) {
      gains51.gains[i] = gains71.gains[i];
    }

    // Fold back channels into surround
    gains51.gains[4] += gains71.gains[6] * 0.707f; // Back Left → Surround Left
    gains51.gains[5] +=
        gains71.gains[7] * 0.707f; // Back Right → Surround Right

    return gains51;
  }

  /**
   * @brief Auto-downmix gains to target layout.
   */
  static SpeakerGains AutoDownmix(const SpeakerGains &source,
                                  SpeakerLayout sourceLayout,
                                  SpeakerLayout targetLayout) {
    if (sourceLayout == targetLayout) {
      return source;
    }

    SpeakerGains result = source;

    // 7.1 → 5.1
    if (sourceLayout == SpeakerLayout::Surround71 &&
        targetLayout == SpeakerLayout::Surround51) {
      return Downmix71To51(source);
    }

    // 5.1 → Stereo
    if (sourceLayout == SpeakerLayout::Surround51 &&
        targetLayout == SpeakerLayout::Stereo) {
      return Downmix51ToStereo(source);
    }

    // 7.1 → Stereo (via 5.1)
    if (sourceLayout == SpeakerLayout::Surround71 &&
        targetLayout == SpeakerLayout::Stereo) {
      return Downmix51ToStereo(Downmix71To51(source));
    }

    return result;
  }
};

/**
 * @brief Get number of channels for a layout.
 */
inline int GetChannelCount(SpeakerLayout layout) {
  return static_cast<int>(layout);
}

/**
 * @brief Get speaker layout from channel count.
 */
inline SpeakerLayout GetLayoutFromChannels(int channels) {
  switch (channels) {
  case 1:
    return SpeakerLayout::Mono;
  case 2:
    return SpeakerLayout::Stereo;
  case 4:
    return SpeakerLayout::Quad;
  case 6:
    return SpeakerLayout::Surround51;
  case 8:
    return SpeakerLayout::Surround71;
  default:
    return SpeakerLayout::Stereo;
  }
}

} // namespace Orpheus
