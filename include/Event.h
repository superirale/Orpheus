/**
 * @file Event.h
 * @brief Audio event playback system.
 *
 * Provides the AudioEvent class for playing registered audio events
 * with automatic bus routing and filter support.
 */
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

/**
 * @brief Callback type for routing audio handles to buses.
 *
 * @param handle The audio handle to route.
 * @param busName The name of the target bus.
 */
using BusRouterCallback = std::function<void(AudioHandle, const std::string &)>;

/**
 * @brief Handles playback of audio events.
 *
 * AudioEvent manages the playback of registered events from the SoundBank,
 * applying randomization (pitch/volume variance), bus routing, and filters.
 */
class AudioEvent {
public:
  /**
   * @brief Construct an AudioEvent handler.
   * @param engine Reference to the SoLoud engine.
   * @param bank Reference to the sound bank for event lookups.
   */
  AudioEvent(SoLoud::Soloud &engine, SoundBank &bank);

  /**
   * @brief Set the callback used to route audio handles to buses.
   * @param router Callback function for bus routing.
   */
  void SetBusRouter(BusRouterCallback router) {
    m_BusRouter = std::move(router);
  }

  /**
   * @brief Play an audio event.
   * @param eventName Name of the registered event.
   * @param busName Name of the bus to route to (default: "Master").
   * @return Handle to the playing audio, or 0 on failure.
   */
  AudioHandle Play(const std::string &eventName,
                   const std::string &busName = "Master");

  /**
   * @brief Get the occlusion filter for external configuration.
   * @return Reference to the biquad filter used for occlusion.
   */
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
