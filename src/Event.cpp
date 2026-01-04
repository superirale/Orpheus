#include "../include/Event.h"
#include "../include/Log.h"

#include <random>
#include <vector>

#include <soloud.h>
#include <soloud_biquadresonantfilter.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

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

// PIMPL implementation struct
struct AudioEventImpl {
  SoLoud::Soloud *engine = nullptr;
  SoundBank *bank = nullptr;
  std::vector<std::shared_ptr<SoLoud::AudioSource>> activeSounds;
  SoLoud::BiquadResonantFilter occlusionFilter;
  BusRouterCallback busRouter;

  AudioEventImpl(SoLoud::Soloud *eng, SoundBank &bk) : engine(eng), bank(&bk) {
    occlusionFilter.setParams(SoLoud::BiquadResonantFilter::LOWPASS, 22000.0f,
                              0.5f);
  }
};

AudioEvent::AudioEvent(NativeEngineHandle engine, SoundBank &bank)
    : m_Impl(std::make_unique<AudioEventImpl>(
          static_cast<SoLoud::Soloud *>(engine.ptr), bank)) {}

AudioEvent::~AudioEvent() = default;

AudioEvent::AudioEvent(AudioEvent &&) noexcept = default;
AudioEvent &AudioEvent::operator=(AudioEvent &&) noexcept = default;

void AudioEvent::SetBusRouter(BusRouterCallback router) {
  m_Impl->busRouter = std::move(router);
}

AudioHandle AudioEvent::Play(const std::string &eventName,
                             const std::string &busName) {
  auto eventResult = m_Impl->bank->FindEvent(eventName);
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
    wavstream->setFilter(0, &m_Impl->occlusionFilter);
    AudioHandle h = m_Impl->engine->play(*wavstream);
    m_Impl->activeSounds.push_back(wavstream);
    m_Impl->engine->setVolume(h, volume);
    m_Impl->engine->setRelativePlaySpeed(h, pitch);
    if (m_Impl->busRouter) {
      m_Impl->busRouter(h, busName);
    }
    return h;
  } else {
    auto wav = std::make_shared<SoLoud::Wav>();
    wav->load(ed.path.c_str());
    wav->setFilter(0, &m_Impl->occlusionFilter);
    AudioHandle h = m_Impl->engine->play(*wav);
    m_Impl->activeSounds.push_back(wav);
    m_Impl->engine->setVolume(h, volume);
    m_Impl->engine->setRelativePlaySpeed(h, pitch);
    if (m_Impl->busRouter) {
      m_Impl->busRouter(h, busName);
    }
    return h;
  }
}

NativeFilterHandle AudioEvent::GetOcclusionFilter() {
  return NativeFilterHandle{&m_Impl->occlusionFilter};
}

} // namespace Orpheus
