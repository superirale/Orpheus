#pragma once

#include "Voice.h"
#include <algorithm>

class VoicePool {
public:
  VoicePool(uint32_t maxRealVoices = 32) : m_MaxRealVoices(maxRealVoices) {}

  // Set max simultaneous real voices
  void SetMaxVoices(uint32_t max) { m_MaxRealVoices = max; }
  uint32_t GetMaxVoices() const { return m_MaxRealVoices; }

  // Set stealing behavior
  void SetStealBehavior(StealBehavior behavior) { m_StealBehavior = behavior; }
  StealBehavior GetStealBehavior() const { return m_StealBehavior; }

  // Allocate a new voice (returns nullptr if can't allocate and steal fails)
  Voice *AllocateVoice(const std::string &eventName, uint8_t priority,
                       const Vector3 &position, float maxDistance) {
    // Find or create a voice slot
    Voice *voice = FindFreeVoice();
    if (!voice) {
      voice = CreateVoice();
    }

    voice->id = m_NextVoiceID++;
    voice->eventName = eventName;
    voice->priority = priority;
    voice->position = position;
    voice->maxDistance = maxDistance;
    voice->playbackTime = 0.0f;
    voice->startTime = m_CurrentTime;
    voice->state = VoiceState::Virtual; // Start as virtual, caller promotes

    return voice;
  }

  // Try to make a voice real (returns true if successful)
  bool MakeReal(Voice *voice) {
    if (!voice || voice->IsReal())
      return voice != nullptr;

    uint32_t realCount = GetRealVoiceCount();
    if (realCount < m_MaxRealVoices) {
      voice->state = VoiceState::Real;
      return true;
    }

    // Need to steal a voice
    Voice *victim = FindVoiceToSteal(voice->priority, voice->audibility);
    if (victim) {
      victim->state = VoiceState::Virtual;
      victim->handle = 0;
      voice->state = VoiceState::Real;
      return true;
    }

    // Couldn't steal, stay virtual
    return false;
  }

  // Make a voice virtual (stop playing but track)
  void MakeVirtual(Voice *voice) {
    if (voice && voice->IsReal()) {
      voice->state = VoiceState::Virtual;
      voice->handle = 0;
    }
  }

  // Stop and release a voice
  void StopVoice(Voice *voice) {
    if (voice) {
      voice->state = VoiceState::Stopped;
      voice->handle = 0;
    }
  }

  // Update all voices (call each frame)
  void Update(float dt, const Vector3 &listenerPos) {
    m_CurrentTime += dt;

    // Update audibility for all active voices
    for (auto &voice : m_Voices) {
      if (voice.IsStopped())
        continue;

      voice.playbackTime += dt;
      voice.UpdateAudibility(listenerPos);
    }

    // Promote virtual voices that are now more audible than real ones
    PromoteVirtualVoices();
  }

  // Get voice counts
  uint32_t GetRealVoiceCount() const {
    return std::count_if(m_Voices.begin(), m_Voices.end(),
                         [](const Voice &v) { return v.IsReal(); });
  }

  uint32_t GetVirtualVoiceCount() const {
    return std::count_if(m_Voices.begin(), m_Voices.end(),
                         [](const Voice &v) { return v.IsVirtual(); });
  }

  uint32_t GetActiveVoiceCount() const {
    return GetRealVoiceCount() + GetVirtualVoiceCount();
  }

  // Get all voices (for iteration)
  std::vector<Voice> &GetVoices() { return m_Voices; }
  const std::vector<Voice> &GetVoices() const { return m_Voices; }

private:
  Voice *FindFreeVoice() {
    for (auto &v : m_Voices) {
      if (v.IsStopped())
        return &v;
    }
    return nullptr;
  }

  Voice *CreateVoice() {
    m_Voices.emplace_back();
    return &m_Voices.back();
  }

  Voice *FindVoiceToSteal(uint8_t newPriority, float newAudibility) {
    Voice *victim = nullptr;
    float victimScore = std::numeric_limits<float>::max();

    for (auto &v : m_Voices) {
      if (!v.IsReal())
        continue;
      // Never steal higher priority
      if (v.priority > newPriority)
        continue;
      // Don't steal if same priority but more audible
      if (v.priority == newPriority && v.audibility >= newAudibility)
        continue;

      float score = 0.0f;
      switch (m_StealBehavior) {
      case StealBehavior::Oldest:
        score = -v.startTime; // Oldest = lowest start time
        break;
      case StealBehavior::Furthest:
        score = -v.audibility; // Use inverse audibility (far = low audibility)
        break;
      case StealBehavior::Quietest:
        score = v.audibility; // Quietest = lowest audibility
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

  void PromoteVirtualVoices() {
    // Sort virtual voices by audibility (highest first)
    std::vector<Voice *> virtualVoices;
    for (auto &v : m_Voices) {
      if (v.IsVirtual())
        virtualVoices.push_back(&v);
    }

    std::sort(virtualVoices.begin(), virtualVoices.end(),
              [](Voice *a, Voice *b) { return a->audibility > b->audibility; });

    // Try to promote each virtual voice
    for (Voice *v : virtualVoices) {
      if (GetRealVoiceCount() >= m_MaxRealVoices)
        break;
      // Only promote if audibility > 0 (in range)
      if (v->audibility > 0.01f) {
        v->state = VoiceState::Real;
        // Note: caller must restart the sound with handle
      }
    }
  }

  std::vector<Voice> m_Voices;
  uint32_t m_MaxRealVoices = 32;
  uint32_t m_NextVoiceID = 1;
  float m_CurrentTime = 0.0f;
  StealBehavior m_StealBehavior = StealBehavior::Quietest;
};
