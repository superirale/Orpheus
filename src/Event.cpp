#include "../include/Event.h"
#include "../include/Log.h"

namespace Orpheus {

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

  float volume = ed.volumeMin;
  float pitch = ed.pitchMin;

  if (ed.stream) {
    auto wavstream = std::make_shared<SoLoud::WavStream>();
    wavstream->load(ed.path.c_str());
    wavstream->setFilter(0, &m_OcclusionFilter);
    AudioHandle h = m_Engine.play(*wavstream);
    m_ActiveSounds.push_back(wavstream);
    m_Engine.setVolume(h, volume);
    m_Engine.setSamplerate(
        h, (unsigned int)(m_Engine.getBackendSamplerate() * pitch));
    RouteHandleToBus(h, busName);
    return h;
  } else {
    auto wav = std::make_shared<SoLoud::Wav>();
    wav->load(ed.path.c_str());
    wav->setFilter(0, &m_OcclusionFilter);
    AudioHandle h = m_Engine.play(*wav);
    m_ActiveSounds.push_back(wav);
    m_Engine.setVolume(h, volume);
    m_Engine.setSamplerate(
        h, (unsigned int)(m_Engine.getBackendSamplerate() * pitch));
    RouteHandleToBus(h, busName);
    return h;
  }
}

SoLoud::BiquadResonantFilter &AudioEvent::GetOcclusionFilter() {
  return m_OcclusionFilter;
}

void AudioEvent::RouteHandleToBus(AudioHandle h, const std::string &busName) {
  (void)h;
  (void)busName;
  
}

} // namespace Orpheus
