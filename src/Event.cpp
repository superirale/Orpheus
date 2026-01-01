#include "../include/Event.h"
#include "../include/Log.h"

#include <random>

namespace Orpheus {

// Thread-local random engine for randomization
static thread_local std::mt19937 s_RandomEngine{std::random_device{}()};

static float RandomFloat(float min, float max) {
  if (min >= max) {
    return min;
  }
  std::uniform_real_distribution<float> dist(min, max);
  return dist(s_RandomEngine);
}

AudioEvent::AudioEvent(SoLoud::Soloud &engine, SoundBank &bank)
    : m_Engine(engine), m_Bank(bank) {
  m_OcclusionFilter.setParams(SoLoud::BiquadResonantFilter::LOWPASS, 22000.0f,
                              0.5f);
}

AudioHandle AudioEvent::Play(const std::string &eventName,
                             const std::string &busName) {
  auto eventResult = m_Bank.FindEvent(eventName);
  if (eventResult.IsError()) {
    ORPHEUS_WARN("Event not found: " << eventName);
    return 0;
  }
  const auto &ed = eventResult.Value();

  // Apply randomization within the specified ranges
  float volume = RandomFloat(ed.volumeMin, ed.volumeMax);
  float pitch = RandomFloat(ed.pitchMin, ed.pitchMax);

  if (ed.stream) {
    auto wavstream = std::make_shared<SoLoud::WavStream>();
    wavstream->load(ed.path.c_str());
    wavstream->setFilter(0, &m_OcclusionFilter);
    AudioHandle h = m_Engine.play(*wavstream);
    m_ActiveSounds.push_back(wavstream);
    m_Engine.setVolume(h, volume);
    m_Engine.setRelativePlaySpeed(h, pitch);
    RouteHandleToBus(h, busName);
    return h;
  } else {
    auto wav = std::make_shared<SoLoud::Wav>();
    wav->load(ed.path.c_str());
    wav->setFilter(0, &m_OcclusionFilter);
    AudioHandle h = m_Engine.play(*wav);
    m_ActiveSounds.push_back(wav);
    m_Engine.setVolume(h, volume);
    m_Engine.setRelativePlaySpeed(h, pitch);
    RouteHandleToBus(h, busName);
    return h;
  }
}

SoLoud::BiquadResonantFilter &AudioEvent::GetOcclusionFilter() {
  return m_OcclusionFilter;
}

void AudioEvent::RouteHandleToBus(AudioHandle h, const std::string &busName) {
  if (m_BusRouter) {
    m_BusRouter(h, busName);
  }
}

} // namespace Orpheus
