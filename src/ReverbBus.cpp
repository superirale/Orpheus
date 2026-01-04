#include "../include/ReverbBus.h"

#include <algorithm>

#include <soloud.h>
#include <soloud_bus.h>
#include <soloud_freeverbfilter.h>

namespace Orpheus {

// PIMPL implementation struct
struct ReverbBusImpl {
  std::string name;
  SoLoud::Bus bus;
  SoLoud::FreeverbFilter reverb;
  SoLoud::Soloud *engine = nullptr;
  SoLoud::handle busHandle = 0;

  float wet = 0.5f;
  float roomSize = 0.5f;
  float damp = 0.5f;
  float width = 1.0f;
  bool freeze = false;
  bool active = false;
};

ReverbBus::ReverbBus(const std::string &name)
    : m_Impl(std::make_unique<ReverbBusImpl>()) {
  m_Impl->name = name;
}

ReverbBus::~ReverbBus() = default;

ReverbBus::ReverbBus(ReverbBus &&) noexcept = default;
ReverbBus &ReverbBus::operator=(ReverbBus &&) noexcept = default;

bool ReverbBus::Init(NativeEngineHandle engine) {
  auto *soloudEngine = static_cast<SoLoud::Soloud *>(engine.ptr);
  if (!soloudEngine)
    return false;

  m_Impl->reverb.setParams(m_Impl->freeze ? 1.0f : 0.0f, m_Impl->roomSize,
                           m_Impl->damp, m_Impl->width);
  m_Impl->bus.setFilter(0, &m_Impl->reverb);

  m_Impl->busHandle = soloudEngine->play(m_Impl->bus);
  if (m_Impl->busHandle == 0) {
    return false;
  }

  soloudEngine->setFilterParameter(m_Impl->busHandle, 0,
                                   SoLoud::FreeverbFilter::WET, m_Impl->wet);

  m_Impl->engine = soloudEngine;
  m_Impl->active = true;
  return true;
}

void ReverbBus::ApplyPreset(ReverbPreset preset) {
  switch (preset) {
  case ReverbPreset::Room:
    SetParams(0.3f, 0.4f, 0.7f, 0.8f);
    break;
  case ReverbPreset::Hall:
    SetParams(0.5f, 0.6f, 0.5f, 1.0f);
    break;
  case ReverbPreset::Cave:
    SetParams(0.6f, 0.85f, 0.3f, 1.0f);
    break;
  case ReverbPreset::Cathedral:
    SetParams(0.7f, 0.95f, 0.2f, 1.0f);
    break;
  case ReverbPreset::Underwater:
    SetParams(0.9f, 0.7f, 0.8f, 0.5f);
    break;
  }
}

void ReverbBus::SetParams(float wet, float roomSize, float damp, float width) {
  m_Impl->wet = std::clamp(wet, 0.0f, 1.0f);
  m_Impl->roomSize = std::clamp(roomSize, 0.0f, 1.0f);
  m_Impl->damp = std::clamp(damp, 0.0f, 1.0f);
  m_Impl->width = std::clamp(width, 0.0f, 1.0f);

  m_Impl->reverb.setParams(m_Impl->freeze ? 1.0f : 0.0f, m_Impl->roomSize,
                           m_Impl->damp, m_Impl->width);

  if (m_Impl->engine && m_Impl->busHandle != 0) {
    m_Impl->engine->setFilterParameter(
        m_Impl->busHandle, 0, SoLoud::FreeverbFilter::WET, m_Impl->wet);
    m_Impl->engine->setFilterParameter(m_Impl->busHandle, 0,
                                       SoLoud::FreeverbFilter::ROOMSIZE,
                                       m_Impl->roomSize);
    m_Impl->engine->setFilterParameter(
        m_Impl->busHandle, 0, SoLoud::FreeverbFilter::DAMP, m_Impl->damp);
    m_Impl->engine->setFilterParameter(
        m_Impl->busHandle, 0, SoLoud::FreeverbFilter::WIDTH, m_Impl->width);
  }
}

void ReverbBus::SetWet(float wet, float fadeTime) {
  m_Impl->wet = std::clamp(wet, 0.0f, 1.0f);
  if (m_Impl->engine && m_Impl->busHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Impl->engine->fadeFilterParameter(m_Impl->busHandle, 0,
                                          SoLoud::FreeverbFilter::WET,
                                          m_Impl->wet, fadeTime);
    } else {
      m_Impl->engine->setFilterParameter(
          m_Impl->busHandle, 0, SoLoud::FreeverbFilter::WET, m_Impl->wet);
    }
  }
}

void ReverbBus::SetRoomSize(float roomSize, float fadeTime) {
  m_Impl->roomSize = std::clamp(roomSize, 0.0f, 1.0f);
  if (m_Impl->engine && m_Impl->busHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Impl->engine->fadeFilterParameter(m_Impl->busHandle, 0,
                                          SoLoud::FreeverbFilter::ROOMSIZE,
                                          m_Impl->roomSize, fadeTime);
    } else {
      m_Impl->engine->setFilterParameter(m_Impl->busHandle, 0,
                                         SoLoud::FreeverbFilter::ROOMSIZE,
                                         m_Impl->roomSize);
    }
  }
}

void ReverbBus::SetDamp(float damp, float fadeTime) {
  m_Impl->damp = std::clamp(damp, 0.0f, 1.0f);
  if (m_Impl->engine && m_Impl->busHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Impl->engine->fadeFilterParameter(m_Impl->busHandle, 0,
                                          SoLoud::FreeverbFilter::DAMP,
                                          m_Impl->damp, fadeTime);
    } else {
      m_Impl->engine->setFilterParameter(
          m_Impl->busHandle, 0, SoLoud::FreeverbFilter::DAMP, m_Impl->damp);
    }
  }
}

void ReverbBus::SetWidth(float width, float fadeTime) {
  m_Impl->width = std::clamp(width, 0.0f, 1.0f);
  if (m_Impl->engine && m_Impl->busHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Impl->engine->fadeFilterParameter(m_Impl->busHandle, 0,
                                          SoLoud::FreeverbFilter::WIDTH,
                                          m_Impl->width, fadeTime);
    } else {
      m_Impl->engine->setFilterParameter(
          m_Impl->busHandle, 0, SoLoud::FreeverbFilter::WIDTH, m_Impl->width);
    }
  }
}

void ReverbBus::SetFreeze(bool freeze) {
  m_Impl->freeze = freeze;
  if (m_Impl->engine && m_Impl->busHandle != 0) {
    m_Impl->engine->setFilterParameter(m_Impl->busHandle, 0,
                                       SoLoud::FreeverbFilter::FREEZE,
                                       freeze ? 1.0f : 0.0f);
  }
}

float ReverbBus::GetWet() const { return m_Impl->wet; }
float ReverbBus::GetRoomSize() const { return m_Impl->roomSize; }
float ReverbBus::GetDamp() const { return m_Impl->damp; }
float ReverbBus::GetWidth() const { return m_Impl->width; }
bool ReverbBus::IsFreeze() const { return m_Impl->freeze; }
bool ReverbBus::IsActive() const { return m_Impl->active; }
const std::string &ReverbBus::GetName() const { return m_Impl->name; }

NativeBusHandle ReverbBus::GetBus() { return NativeBusHandle{&m_Impl->bus}; }
AudioHandle ReverbBus::GetBusHandle() const {
  return static_cast<AudioHandle>(m_Impl->busHandle);
}

void ReverbBus::SendToReverb(NativeEngineHandle engine, AudioHandle audioHandle,
                             float sendLevel) {
  if (!m_Impl->active || m_Impl->busHandle == 0)
    return;
  (void)engine;
  (void)audioHandle;
  (void)sendLevel;
}

} // namespace Orpheus
