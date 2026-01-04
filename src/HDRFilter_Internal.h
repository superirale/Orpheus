/**
 * @file HDRFilter_Internal.h
 * @brief Internal SoLoud filter implementation for HDR Audio.
 *
 * This header is used internally by AudioManager to integrate the
 * HDRMixer with SoLoud's filter system. Do not include in user code.
 */
#pragma once

#include "../include/HDRAudio.h"

#include <soloud.h>
#include <soloud_filter.h>

namespace Orpheus {

/**
 * @brief SoLoud filter instance that wraps HDRMixer for real audio processing.
 */
class HDRFilterInstance : public SoLoud::FilterInstance {
public:
  explicit HDRFilterInstance(HDRMixer *mixer) : m_Mixer(mixer) {}

  void filterChannel(float *aBuffer, unsigned int aSamples, float aSamplerate,
                     SoLoud::time aTime, unsigned int aChannel,
                     unsigned int aChannels) override {
    (void)aSamplerate;
    (void)aTime;
    (void)aChannels;
    // Only process left channel (channel 0) to avoid double processing
    // HDRMixer handles mono processing for LUFS measurement
    if (aChannel == 0 && m_Mixer) {
      m_Mixer->Process(aBuffer, aSamples);
    }
  }

private:
  HDRMixer *m_Mixer;
};

/**
 * @brief SoLoud filter that provides HDR audio processing.
 */
class HDRFilter : public SoLoud::Filter {
public:
  explicit HDRFilter(HDRMixer *mixer) : m_Mixer(mixer) {}

  SoLoud::FilterInstance *createInstance() override {
    return new HDRFilterInstance(m_Mixer);
  }

private:
  HDRMixer *m_Mixer;
};

} // namespace Orpheus
