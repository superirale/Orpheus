#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <soloud.h>
#include <soloud_biquadresonantfilter.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include "SoundBank.h"
#include "Types.h"

namespace Orpheus {

/// Callback type for routing audio handles to buses
using BusRouterCallback = std::function<void(AudioHandle, const std::string &)>;

class AudioEvent {
public:
  AudioEvent(SoLoud::Soloud &engine, SoundBank &bank);

  /// Set the callback used to route audio handles to buses
  void SetBusRouter(BusRouterCallback router) {
    m_BusRouter = std::move(router);
  }

  AudioHandle Play(const std::string &eventName,
                   const std::string &busName = "Master");

  SoLoud::BiquadResonantFilter &GetOcclusionFilter();

private:
  void RouteHandleToBus(AudioHandle h, const std::string &busName);

  SoLoud::Soloud &m_Engine;
  SoundBank &m_Bank;
  std::vector<std::shared_ptr<SoLoud::AudioSource>> m_ActiveSounds;
  SoLoud::BiquadResonantFilter m_OcclusionFilter;
  BusRouterCallback m_BusRouter;
};

} // namespace Orpheus
