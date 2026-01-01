/**
 * @file Bus.h
 * @brief Audio bus for grouping and processing sounds.
 *
 * Provides the Bus class for routing audio through shared processing
 * chains and volume control.
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <soloud.h>
#include <soloud_bus.h>

#include "Compressor.h"

namespace Orpheus {

/**
 * @brief Audio bus for grouping and processing sounds.
 *
 * Buses allow routing multiple sounds through shared volume and filter
 * processing. Common uses include Music, SFX, and Ambient buses with
 * independent volume control.
 *
 * @par Example Usage:
 * @code
 * // Create buses
 * audio.CreateBus("Music");
 * audio.CreateBus("SFX");
 *
 * // Set volumes
 * auto music = audio.GetBus("Music");
 * if (music) music.Value()->SetVolume(0.8f);
 * @endcode
 */
class Bus {
public:
  /**
   * @brief Construct a named bus.
   * @param name Unique name for this bus.
   */
  Bus(const std::string &name);

  /**
   * @brief Add a filter to this bus's processing chain.
   * @param f Shared pointer to the filter.
   */
  void AddFilter(std::shared_ptr<SoLoud::Filter> f);

  /**
   * @brief Route an audio handle through this bus.
   * @param engine Reference to the SoLoud engine.
   * @param h Audio handle to route.
   */
  void AddHandle(SoLoud::Soloud &engine, SoLoud::handle h);

  /**
   * @brief Update bus state (volume fading).
   * @param dt Delta time in seconds.
   */
  void Update(float dt);

  /**
   * @brief Set the bus volume immediately.
   * @param v Volume level (0.0-1.0).
   */
  void SetVolume(float v);

  /**
   * @brief Set a target volume with fade transition.
   * @param v Target volume (0.0-1.0).
   * @param fadeSeconds Duration of fade (0 = instant).
   */
  void SetTargetVolume(float v, float fadeSeconds = 0.0f);

  /**
   * @brief Get the current volume.
   * @return Current volume level.
   */
  [[nodiscard]] float GetVolume() const;

  /**
   * @brief Get the target volume.
   * @return Target volume level.
   */
  [[nodiscard]] float GetTargetVolume() const;

  /**
   * @brief Get the bus name.
   * @return Reference to the bus name.
   */
  [[nodiscard]] const std::string &GetName() const;

  /**
   * @brief Get raw SoLoud bus pointer for advanced usage.
   * @return Pointer to the underlying SoLoud::Bus.
   */
  [[nodiscard]] SoLoud::Bus *Raw();

  /**
   * @brief Set compressor settings for this bus.
   * @param settings Compressor configuration.
   */
  void SetCompressor(const CompressorSettings &settings);

  /**
   * @brief Enable/disable the compressor.
   * @param enabled true to enable compression.
   */
  void SetCompressorEnabled(bool enabled);

  /**
   * @brief Check if compressor is enabled.
   */
  [[nodiscard]] bool IsCompressorEnabled() const;

  /**
   * @brief Get the current compressor settings.
   */
  [[nodiscard]] const CompressorSettings &GetCompressorSettings() const;

  /**
   * @brief Get the current gain reduction in dB.
   */
  [[nodiscard]] float GetCompressorGainReduction() const;

private:
  std::unique_ptr<SoLoud::Bus> m_Bus;
  std::string m_Name;
  float m_Volume = 1.0f;
  float m_TargetVolume = 1.0f;
  float m_StartVolume = 1.0f;
  float m_FadeTime = 0.0f;
  std::vector<std::shared_ptr<SoLoud::Filter>> m_Filters;
  std::vector<SoLoud::handle> m_Handles;
  SoLoud::Soloud *m_Engine = nullptr;
  Compressor m_Compressor;
};

} // namespace Orpheus
