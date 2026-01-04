/**
 * @file ReverbBus.h
 * @brief Reverb bus for spatial audio environment simulation.
 *
 * Provides ReverbBus class for applying reverb effects to audio
 * routed through send levels, simulating acoustic environments.
 */
#pragma once

#include <memory>
#include <string>

#include "OpaqueHandles.h"
#include "Types.h"

namespace Orpheus {

// Forward declaration for PIMPL
struct ReverbBusImpl;

/**
 * @brief Preset reverb configurations.
 */
enum class ReverbPreset {
  Room,      ///< Small room reverb
  Hall,      ///< Large hall reverb
  Cave,      ///< Cave-like reverb with long decay
  Cathedral, ///< Very large space with rich reverb
  Underwater ///< Muffled underwater effect
};

/**
 * @brief Audio bus with integrated reverb effect.
 *
 * ReverbBus combines an audio bus with a reverb filter to simulate
 * acoustic environments. Sounds are routed to the reverb bus via send
 * levels, allowing multiple sounds to share reverb processing efficiently.
 *
 * @par Example Usage:
 * @code
 * // Create a cave reverb
 * audio.CreateReverbBus("CaveReverb", ReverbPreset::Cave);
 *
 * // Or with custom parameters
 * audio.CreateReverbBus("CustomReverb", 0.8f, 0.3f, 0.6f, 1.0f);
 * @endcode
 */
class ReverbBus {
public:
  /**
   * @brief Construct a named reverb bus.
   * @param name Unique name for this reverb bus.
   */
  ReverbBus(const std::string &name);

  /**
   * @brief Destructor.
   */
  ~ReverbBus();

  // Non-copyable, movable
  ReverbBus(const ReverbBus &) = delete;
  ReverbBus &operator=(const ReverbBus &) = delete;
  ReverbBus(ReverbBus &&) noexcept;
  ReverbBus &operator=(ReverbBus &&) noexcept;

  /**
   * @brief Initialize the reverb bus with the audio engine.
   * @param engine Native engine handle.
   * @return true if initialization succeeded.
   */
  [[nodiscard]] bool Init(NativeEngineHandle engine);

  /**
   * @brief Apply a reverb preset.
   * @param preset The preset to apply.
   */
  void ApplyPreset(ReverbPreset preset);

  /**
   * @brief Set all reverb parameters at once.
   * @param wet Wet/dry mix (0.0-1.0).
   * @param roomSize Room size (0.0-1.0).
   * @param damp High frequency damping (0.0-1.0).
   * @param width Stereo width (0.0-1.0).
   */
  void SetParams(float wet, float roomSize, float damp, float width);

  /**
   * @brief Set the wet/dry mix.
   * @param wet Wet level (0.0-1.0).
   * @param fadeTime Fade transition time in seconds.
   */
  void SetWet(float wet, float fadeTime = 0.0f);

  /**
   * @brief Set the room size.
   * @param roomSize Room size (0.0-1.0).
   * @param fadeTime Fade transition time in seconds.
   */
  void SetRoomSize(float roomSize, float fadeTime = 0.0f);

  /**
   * @brief Set high frequency damping.
   * @param damp Damping amount (0.0-1.0).
   * @param fadeTime Fade transition time in seconds.
   */
  void SetDamp(float damp, float fadeTime = 0.0f);

  /**
   * @brief Set stereo width.
   * @param width Stereo width (0.0-1.0).
   * @param fadeTime Fade transition time in seconds.
   */
  void SetWidth(float width, float fadeTime = 0.0f);

  /**
   * @brief Set freeze mode (infinite reverb tail).
   * @param freeze true to enable freeze.
   */
  void SetFreeze(bool freeze);

  /// @name Getters
  /// @{
  [[nodiscard]] float GetWet() const;
  [[nodiscard]] float GetRoomSize() const;
  [[nodiscard]] float GetDamp() const;
  [[nodiscard]] float GetWidth() const;
  [[nodiscard]] bool IsFreeze() const;
  [[nodiscard]] bool IsActive() const;
  [[nodiscard]] const std::string &GetName() const;
  /// @}

  /**
   * @brief Get native bus handle for advanced usage.
   * @return Opaque handle to the underlying native bus.
   */
  [[nodiscard]] NativeBusHandle GetBus();

  /**
   * @brief Get the bus handle.
   * @return Audio handle for the bus.
   */
  [[nodiscard]] AudioHandle GetBusHandle() const;

  /**
   * @brief Send audio to this reverb bus.
   * @param engine Native engine handle.
   * @param audioHandle Handle of the audio to send.
   * @param sendLevel Send level (0.0-1.0).
   */
  void SendToReverb(NativeEngineHandle engine, AudioHandle audioHandle,
                    float sendLevel);

private:
  std::unique_ptr<ReverbBusImpl> m_Impl;
};

} // namespace Orpheus
