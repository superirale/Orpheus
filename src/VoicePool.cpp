#include "../include/VoicePool.h"

namespace Orpheus {

VoicePool::VoicePool(uint32_t maxRealVoices) : m_MaxRealVoices(maxRealVoices) {}

void VoicePool::SetMaxVoices(uint32_t max) { m_MaxRealVoices = max; }
uint32_t VoicePool::GetMaxVoices() const { return m_MaxRealVoices; }

void VoicePool::SetStealBehavior(StealBehavior behavior) {
  m_StealBehavior = behavior;
}
StealBehavior VoicePool::GetStealBehavior() const { return m_StealBehavior; }

Voice *VoicePool::AllocateVoice(const std::string &eventName, uint8_t priority,
                                const Vector3 &position,
                                const DistanceSettings &distanceSettings) {
  Voice *voice = FindFreeVoice();
  if (!voice) {
    voice = CreateVoice();
  }

  voice->id = m_NextVoiceID++;
  voice->eventName = eventName;
  voice->priority = priority;
  voice->position = position;
  voice->distanceSettings = distanceSettings;
  voice->playbackTime = 0.0f;
  voice->startTime = m_CurrentTime;
  voice->state = VoiceState::Virtual;

  return voice;
}

bool VoicePool::MakeReal(Voice *voice) {
  if (!voice || voice->IsReal())
    return voice != nullptr;

  uint32_t realCount = GetRealVoiceCount();
  if (realCount < m_MaxRealVoices) {
    voice->state = VoiceState::Real;
    return true;
  }

  Voice *victim = FindVoiceToSteal(voice->priority, voice->audibility);
  if (victim) {
    victim->state = VoiceState::Virtual;
    victim->handle = 0;
    voice->state = VoiceState::Real;
    return true;
  }

  return false;
}

void VoicePool::MakeVirtual(Voice *voice) {
  if (voice && voice->IsReal()) {
    voice->state = VoiceState::Virtual;
    voice->handle = 0;
  }
}

void VoicePool::StopVoice(Voice *voice) {
  if (voice) {
    voice->state = VoiceState::Stopped;
    voice->handle = 0;
  }
}

void VoicePool::Update(float dt, const Vector3 &listenerPos) {
  m_CurrentTime += dt;

  for (auto &voicePtr : m_Voices) {
    if (voicePtr->IsStopped())
      continue;

    voicePtr->playbackTime += dt;
    voicePtr->UpdateAudibility(listenerPos);
  }

  PromoteVirtualVoices();
}

uint32_t VoicePool::GetRealVoiceCount() const {
  return std::count_if(
      m_Voices.begin(), m_Voices.end(),
      [](const std::unique_ptr<Voice> &v) { return v->IsReal(); });
}

uint32_t VoicePool::GetVirtualVoiceCount() const {
  return std::count_if(
      m_Voices.begin(), m_Voices.end(),
      [](const std::unique_ptr<Voice> &v) { return v->IsVirtual(); });
}

uint32_t VoicePool::GetActiveVoiceCount() const {
  return GetRealVoiceCount() + GetVirtualVoiceCount();
}

Voice *VoicePool::GetVoiceAt(size_t index) {
  if (index < m_Voices.size()) {
    return m_Voices[index].get();
  }
  return nullptr;
}

const Voice *VoicePool::GetVoiceAt(size_t index) const {
  if (index < m_Voices.size()) {
    return m_Voices[index].get();
  }
  return nullptr;
}

size_t VoicePool::GetVoiceCount() const { return m_Voices.size(); }

Voice *VoicePool::FindFreeVoice() {
  for (auto &voicePtr : m_Voices) {
    if (voicePtr->IsStopped())
      return voicePtr.get();
  }
  return nullptr;
}

Voice *VoicePool::CreateVoice() {
  m_Voices.push_back(std::make_unique<Voice>());
  return m_Voices.back().get();
}

Voice *VoicePool::FindVoiceToSteal(uint8_t newPriority, float newAudibility) {
  Voice *victim = nullptr;
  float victimScore = std::numeric_limits<float>::max();

  for (auto &voicePtr : m_Voices) {
    Voice &v = *voicePtr;
    if (!v.IsReal())
      continue;
    if (v.priority > newPriority)
      continue;
    if (v.priority == newPriority && v.audibility >= newAudibility)
      continue;

    float score = 0.0f;
    switch (m_StealBehavior) {
    case StealBehavior::Oldest:
      score = -v.startTime;
      break;
    case StealBehavior::Furthest:
      score = -v.audibility;
      break;
    case StealBehavior::Quietest:
      score = v.audibility;
      break;
    case StealBehavior::None:
      return nullptr;
    }

    if (score < victimScore || (victim && v.priority < victim->priority)) {
      victim = &v;
      victimScore = score;
    }
  }
  return victim;
}

void VoicePool::PromoteVirtualVoices() {
  std::vector<Voice *> virtualVoices;
  for (auto &voicePtr : m_Voices) {
    if (voicePtr->IsVirtual())
      virtualVoices.push_back(voicePtr.get());
  }

  std::sort(virtualVoices.begin(), virtualVoices.end(),
            [](Voice *a, Voice *b) { return a->audibility > b->audibility; });

  for (Voice *v : virtualVoices) {
    if (GetRealVoiceCount() >= m_MaxRealVoices)
      break;
    if (v->audibility > 0.01f) {
      v->state = VoiceState::Real;
    }
  }
}

} // namespace Orpheus
