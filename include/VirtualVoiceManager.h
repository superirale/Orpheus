/**
 * @file VirtualVoiceManager.h
 * @brief Legacy virtual voice manager (deprecated).
 *
 * Simple voice limit management. Prefer VoicePool for full
 * virtual voice system functionality.
 *
 * @deprecated Use VoicePool instead.
 */
#pragma once

#include <algorithm>
#include <vector>

#include "Types.h"

namespace Orpheus {

/**
 * @brief Simple virtual voice manager with handle tracking.
 *
 * Maintains a list of active handles up to a configured limit.
 *
 * @deprecated This is a legacy class. Use VoicePool for more
 *             sophisticated virtual voice management.
 */
class VirtualVoiceManager {
public:
  /**
   * @brief Construct with a voice limit.
   * @param limit Maximum number of concurrent voices.
   */
  VirtualVoiceManager(size_t limit) : mLimit(limit) {}

  /**
   * @brief Check if a new voice can be played.
   * @return true if under the voice limit.
   */
  [[nodiscard]] bool canPlay() const { return mActiveHandles.size() < mLimit; }

  /**
   * @brief Register an active audio handle.
   * @param h The handle to register.
   */
  void registerHandle(AudioHandle h) { mActiveHandles.push_back(h); }

  /**
   * @brief Unregister an audio handle when playback stops.
   * @param h The handle to unregister.
   */
  void unregisterHandle(AudioHandle h) {
    mActiveHandles.erase(
        std::remove(mActiveHandles.begin(), mActiveHandles.end(), h),
        mActiveHandles.end());
  }

private:
  size_t mLimit;
  std::vector<AudioHandle> mActiveHandles;
};

} // namespace Orpheus
