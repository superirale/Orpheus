#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "Voice.h"

namespace Orpheus {

class VoicePool {
public:
  VoicePool(uint32_t maxRealVoices = 32);

  void SetMaxVoices(uint32_t max);
  uint32_t GetMaxVoices() const;

  void SetStealBehavior(StealBehavior behavior);
  StealBehavior GetStealBehavior() const;

  Voice *AllocateVoice(const std::string &eventName, uint8_t priority,
                       const Vector3 &position, float maxDistance);

  bool MakeReal(Voice *voice);
  void MakeVirtual(Voice *voice);
  void StopVoice(Voice *voice);

  void Update(float dt, const Vector3 &listenerPos);

  uint32_t GetRealVoiceCount() const;
  uint32_t GetVirtualVoiceCount() const;
  uint32_t GetActiveVoiceCount() const;

  std::vector<Voice> &GetVoices();
  const std::vector<Voice> &GetVoices() const;

private:
  Voice *FindFreeVoice();
  Voice *CreateVoice();
  Voice *FindVoiceToSteal(uint8_t newPriority, float newAudibility);
  void PromoteVirtualVoices();

  std::vector<Voice> m_Voices;
  uint32_t m_MaxRealVoices = 32;
  uint32_t m_NextVoiceID = 1;
  float m_CurrentTime = 0.0f;
  StealBehavior m_StealBehavior = StealBehavior::Quietest;
};

} // namespace Orpheus
