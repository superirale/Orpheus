#include "../include/Bus.h"

#include <soloud.h>
#include <soloud_bus.h>

#include <vector>

namespace Orpheus {

// PIMPL implementation struct
struct BusImpl {
  std::unique_ptr<SoLoud::Bus> bus;
  std::vector<SoLoud::handle> handles;
  SoLoud::Soloud *engine = nullptr;
  float volume = 1.0f;
  float targetVolume = 1.0f;
  float startVolume = 1.0f;
  float fadeTime = 0.0f;

  BusImpl() : bus(std::make_unique<SoLoud::Bus>()) {}
};

Bus::Bus(const std::string &name)
    : m_Impl(std::make_unique<BusImpl>()), m_Name(name) {}

Bus::~Bus() = default;

Bus::Bus(Bus &&) noexcept = default;
Bus &Bus::operator=(Bus &&) noexcept = default;

void Bus::AddHandle(NativeEngineHandle engine, AudioHandle h) {
  m_Impl->engine = static_cast<SoLoud::Soloud *>(engine.ptr);
  m_Impl->handles.push_back(static_cast<SoLoud::handle>(h));
}

void Bus::Update(float dt) {
  if (m_Impl->fadeTime > 0.0f) {
    float step =
        (m_Impl->targetVolume - m_Impl->startVolume) * (dt / m_Impl->fadeTime);
    if ((m_Impl->targetVolume > m_Impl->startVolume &&
         m_Impl->volume + step >= m_Impl->targetVolume) ||
        (m_Impl->targetVolume < m_Impl->startVolume &&
         m_Impl->volume + step <= m_Impl->targetVolume) ||
        (m_Impl->targetVolume == m_Impl->startVolume)) {
      m_Impl->volume = m_Impl->targetVolume;
      m_Impl->fadeTime = 0.0f;
    } else {
      m_Impl->volume += step;
    }
  }

  if (m_Impl->engine) {
    for (auto it = m_Impl->handles.begin(); it != m_Impl->handles.end();) {
      if (m_Impl->engine->isValidVoiceHandle(*it)) {
        m_Impl->engine->setVolume(*it, m_Impl->volume);
        ++it;
      } else {
        it = m_Impl->handles.erase(it);
      }
    }
  }
}

void Bus::SetVolume(float v) {
  m_Impl->volume = v;
  m_Impl->targetVolume = v;
  m_Impl->fadeTime = 0.0f;
}

void Bus::SetTargetVolume(float v, float fadeSeconds) {
  m_Impl->startVolume = m_Impl->volume;
  m_Impl->targetVolume = v;
  m_Impl->fadeTime = fadeSeconds > 0.0f ? fadeSeconds : 0.001f;
}

float Bus::GetVolume() const { return m_Impl->volume; }
float Bus::GetTargetVolume() const { return m_Impl->targetVolume; }
const std::string &Bus::GetName() const { return m_Name; }

NativeBusHandle Bus::Raw() { return NativeBusHandle{m_Impl->bus.get()}; }

// Compressor methods
void Bus::SetCompressor(const CompressorSettings &settings) {
  m_Compressor.SetSettings(settings);
}

void Bus::SetCompressorEnabled(bool enabled) {
  m_Compressor.SetEnabled(enabled);
}

bool Bus::IsCompressorEnabled() const { return m_Compressor.IsEnabled(); }

const CompressorSettings &Bus::GetCompressorSettings() const {
  return m_Compressor.GetSettings();
}

float Bus::GetCompressorGainReduction() const {
  return m_Compressor.GetGainReduction();
}

} // namespace Orpheus
