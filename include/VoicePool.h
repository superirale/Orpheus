/**
 * @file VoicePool.h
 * @brief Voice pool management for the virtual voice system.
 *
 * Manages allocation and lifecycle of Voice objects, including
 * real/virtual state transitions and voice stealing.
 */
#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "Voice.h"

namespace Orpheus {

/**
 * @brief Manages a pool of voices for audio playback.
 *
 * Handles voice allocation, virtual/real state transitions, and
 * voice stealing when the hardware voice limit is exceeded.
 * Implements a virtual voice system where sounds beyond the limit
 * are tracked but not played, allowing them to resume when resources
 * become available.
 */
class VoicePool {
public:
  /**
   * @brief Construct a voice pool with a maximum number of real voices.
   * @param maxRealVoices Maximum concurrent hardware voices (default: 32).
   */
  VoicePool(uint32_t maxRealVoices = 32);

  /**
   * @brief Set the maximum number of real (hardware) voices.
   * @param max Maximum voice count.
   */
  void SetMaxVoices(uint32_t max);

  /**
   * @brief Get the maximum number of real voices.
   * @return Maximum voice count.
   */
  [[nodiscard]] uint32_t GetMaxVoices() const;

  /**
   * @brief Set the behavior when voice limit is exceeded.
   * @param behavior The steal behavior to use.
   */
  void SetStealBehavior(StealBehavior behavior);

  /**
   * @brief Get the current steal behavior.
   * @return Current StealBehavior.
   */
  [[nodiscard]] StealBehavior GetStealBehavior() const;

  /**
   * @brief Allocate a new voice for an event.
   * @param eventName Name of the audio event.
   * @param priority Priority level (0-255).
   * @param position 3D position in world space.
   * @param distanceSettings Distance attenuation settings.
   * @return Pointer to allocated Voice, or nullptr if failed.
   */
  [[nodiscard]] Voice *AllocateVoice(const std::string &eventName,
                                     uint8_t priority, const Vector3 &position,
                                     const DistanceSettings &distanceSettings);

  /**
   * @brief Transition a voice from virtual to real.
   * @param voice Pointer to the voice.
   * @return true if successfully made real.
   */
  [[nodiscard]] bool MakeReal(Voice *voice);

  /**
   * @brief Transition a voice from real to virtual.
   * @param voice Pointer to the voice.
   */
  void MakeVirtual(Voice *voice);

  /**
   * @brief Stop a voice and mark it as available.
   * @param voice Pointer to the voice.
   */
  void StopVoice(Voice *voice);

  /**
   * @brief Update all voices (audibility, state transitions).
   * @param dt Delta time in seconds.
   * @param listenerPos Current listener position.
   */
  void Update(float dt, const Vector3 &listenerPos);

  /**
   * @brief Get count of currently playing real voices.
   * @return Number of real voices.
   */
  [[nodiscard]] uint32_t GetRealVoiceCount() const;

  /**
   * @brief Get count of virtual voices.
   * @return Number of virtual voices.
   */
  [[nodiscard]] uint32_t GetVirtualVoiceCount() const;

  /**
   * @brief Get count of all active voices (real + virtual).
   * @return Total active voice count.
   */
  [[nodiscard]] uint32_t GetActiveVoiceCount() const;

  /**
   * @brief Get voice by index for iteration.
   * @param index Voice index.
   * @return Pointer to voice, or nullptr if out of range.
   */
  [[nodiscard]] Voice *GetVoiceAt(size_t index);

  /**
   * @brief Get voice by index for iteration (const).
   * @param index Voice index.
   * @return Const pointer to voice, or nullptr if out of range.
   */
  [[nodiscard]] const Voice *GetVoiceAt(size_t index) const;

  /**
   * @brief Get total number of voice slots.
   * @return Number of voices in pool.
   */
  [[nodiscard]] size_t GetVoiceCount() const;

private:
  Voice *FindFreeVoice();
  Voice *CreateVoice();
  Voice *FindVoiceToSteal(uint8_t newPriority, float newAudibility);
  void PromoteVirtualVoices();

  std::vector<std::unique_ptr<Voice>> m_Voices;
  uint32_t m_MaxRealVoices = 32;
  uint32_t m_NextVoiceID = 1;
  float m_CurrentTime = 0.0f;
  StealBehavior m_StealBehavior = StealBehavior::Quietest;
};

} // namespace Orpheus
