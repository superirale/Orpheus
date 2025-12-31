#pragma once

#include <algorithm>
#include <vector>

#include "Types.h"

namespace Orpheus {

class VirtualVoiceManager {
public:
  VirtualVoiceManager(size_t limit) : mLimit(limit) {}
  bool canPlay() const { return mActiveHandles.size() < mLimit; }
  void registerHandle(AudioHandle h) { mActiveHandles.push_back(h); }
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
