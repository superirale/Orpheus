#include "../include/ReverbBus.h"

namespace Orpheus {

ReverbBus::ReverbBus(const std::string &name) : m_Name(name) {}

bool ReverbBus::Init(SoLoud::Soloud &engine) {
  m_Reverb.setParams(m_Freeze ? 1.0f : 0.0f, m_RoomSize, m_Damp, m_Width);
  m_Bus.setFilter(0, &m_Reverb);

  m_BusHandle = engine.play(m_Bus);
  if (m_BusHandle == 0) {
    return false;
  }

  engine.setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WET, m_Wet);

  m_Engine = &engine;
  m_Active = true;
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
  m_Wet = std::clamp(wet, 0.0f, 1.0f);
  m_RoomSize = std::clamp(roomSize, 0.0f, 1.0f);
  m_Damp = std::clamp(damp, 0.0f, 1.0f);
  m_Width = std::clamp(width, 0.0f, 1.0f);

  m_Reverb.setParams(m_Freeze ? 1.0f : 0.0f, m_RoomSize, m_Damp, m_Width);

  if (m_Engine && m_BusHandle != 0) {
    m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WET,
                                 m_Wet);
    m_Engine->setFilterParameter(m_BusHandle, 0,
                                 SoLoud::FreeverbFilter::ROOMSIZE, m_RoomSize);
    m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::DAMP,
                                 m_Damp);
    m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WIDTH,
                                 m_Width);
  }
}

void ReverbBus::SetWet(float wet, float fadeTime) {
  m_Wet = std::clamp(wet, 0.0f, 1.0f);
  if (m_Engine && m_BusHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Engine->fadeFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WET,
                                    m_Wet, fadeTime);
    } else {
      m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WET,
                                   m_Wet);
    }
  }
}

void ReverbBus::SetRoomSize(float roomSize, float fadeTime) {
  m_RoomSize = std::clamp(roomSize, 0.0f, 1.0f);
  if (m_Engine && m_BusHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Engine->fadeFilterParameter(m_BusHandle, 0,
                                    SoLoud::FreeverbFilter::ROOMSIZE,
                                    m_RoomSize, fadeTime);
    } else {
      m_Engine->setFilterParameter(
          m_BusHandle, 0, SoLoud::FreeverbFilter::ROOMSIZE, m_RoomSize);
    }
  }
}

void ReverbBus::SetDamp(float damp, float fadeTime) {
  m_Damp = std::clamp(damp, 0.0f, 1.0f);
  if (m_Engine && m_BusHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Engine->fadeFilterParameter(
          m_BusHandle, 0, SoLoud::FreeverbFilter::DAMP, m_Damp, fadeTime);
    } else {
      m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::DAMP,
                                   m_Damp);
    }
  }
}

void ReverbBus::SetWidth(float width, float fadeTime) {
  m_Width = std::clamp(width, 0.0f, 1.0f);
  if (m_Engine && m_BusHandle != 0) {
    if (fadeTime > 0.0f) {
      m_Engine->fadeFilterParameter(
          m_BusHandle, 0, SoLoud::FreeverbFilter::WIDTH, m_Width, fadeTime);
    } else {
      m_Engine->setFilterParameter(m_BusHandle, 0,
                                   SoLoud::FreeverbFilter::WIDTH, m_Width);
    }
  }
}

void ReverbBus::SetFreeze(bool freeze) {
  m_Freeze = freeze;
  if (m_Engine && m_BusHandle != 0) {
    m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::FREEZE,
                                 freeze ? 1.0f : 0.0f);
  }
}

float ReverbBus::GetWet() const { return m_Wet; }
float ReverbBus::GetRoomSize() const { return m_RoomSize; }
float ReverbBus::GetDamp() const { return m_Damp; }
float ReverbBus::GetWidth() const { return m_Width; }
bool ReverbBus::IsFreeze() const { return m_Freeze; }
bool ReverbBus::IsActive() const { return m_Active; }
const std::string &ReverbBus::GetName() const { return m_Name; }

SoLoud::Bus &ReverbBus::GetBus() { return m_Bus; }
SoLoud::handle ReverbBus::GetBusHandle() const { return m_BusHandle; }

void ReverbBus::SendToReverb(SoLoud::Soloud &engine, SoLoud::handle audioHandle,
                             float sendLevel) {
  if (!m_Active || m_BusHandle == 0)
    return;
  (void)engine;
  (void)audioHandle;
  (void)sendLevel;
}

} // namespace Orpheus
