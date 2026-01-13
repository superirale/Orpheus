/**
 * @file Event.h
 * @brief Audio event playback system.
 *
 * Provides the AudioEvent class for playing registered audio events
 * with automatic bus routing and filter support.
 */
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "OpaqueHandles.h"
#include "SoundBank.h"
#include "Types.h"

namespace Orpheus {

// Forward declaration for PIMPL
struct AudioEventImpl;

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
   * @param engine Native engine handle.
   * @param bank Reference to the sound bank for event lookups.
   */
  AudioEvent(NativeEngineHandle engine, SoundBank &bank);

  /**
   * @brief Destructor.
   */
  ~AudioEvent();

  // Non-copyable, movable
  AudioEvent(const AudioEvent &) = delete;
  AudioEvent &operator=(const AudioEvent &) = delete;
  AudioEvent(AudioEvent &&) noexcept;
  AudioEvent &operator=(AudioEvent &&) noexcept;

  /**
   * @brief Set the callback used to route audio handles to buses.
   * @param router Callback function for bus routing.
   */
  void SetBusRouter(BusRouterCallback router);

  /**
   * @brief Play an audio event.
   * @param eventName Name of the registered event.
   * @param busName Name of the bus to route to (default: "Master").
   * @return Handle to the playing audio, or 0 on failure.
   */
  [[nodiscard]] AudioHandle Play(const std::string &eventName,
                                 const std::string &busName = "Master");

  /**
   * @brief Play a specific sound file using settings from an event descriptor.
   * @param path Path to the sound file.
   * @param ed The event descriptor for settings (volume, pitch, etc.).
   * @return Handle to the playing audio, or 0 on failure.
   */
  [[nodiscard]] AudioHandle PlayFromEvent(const std::string &path,
                                          const EventDescriptor &ed);

  /**
   * @brief Get native occlusion filter handle for advanced usage.
   * @return Opaque handle to the underlying filter.
   */
  [[nodiscard]] NativeFilterHandle GetOcclusionFilter();

private:
  std::unique_ptr<AudioEventImpl> m_Impl;
};

} // namespace Orpheus
