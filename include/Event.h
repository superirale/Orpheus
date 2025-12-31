#pragma once

#include "SoundBank.h"
#include "Types.h"

class AudioEvent {
public:
  AudioEvent(SoLoud::Soloud &engine, SoundBank &bank);

  AudioHandle Play(const std::string &eventName,
                   const std::string &busName = "Master");

  SoLoud::BiquadResonantFilter &GetOcclusionFilter();

private:
  void RouteHandleToBus(AudioHandle h, const std::string &busName);

  SoLoud::Soloud &m_Engine;
  SoundBank &m_Bank;
  std::vector<std::shared_ptr<SoLoud::AudioSource>> m_ActiveSounds;
  SoLoud::BiquadResonantFilter m_OcclusionFilter;
};
