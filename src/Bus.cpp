#include "../include/Bus.h"

namespace Orpheus {

Bus::Bus(const std::string &name) : m_Name(name) {
  m_Bus.reset(new SoLoud::Bus());
}

void Bus::AddFilter(std::shared_ptr<SoLoud::Filter> f) { filters.push_back(f); }

void Bus::AddHandle(SoLoud::Soloud &engine, SoLoud::handle h) {
  m_Engine = &engine;
  m_Handles.push_back(h);
}

void Bus::Update(float dt) {
  if (m_FadeTime > 0.0f) {
    float step = (m_TargetVolume - m_StartVolume) * (dt / m_FadeTime);
    if ((m_TargetVolume > m_StartVolume && m_Volume + step >= m_TargetVolume) ||
        (m_TargetVolume < m_StartVolume && m_Volume + step <= m_TargetVolume) ||
        (m_TargetVolume == m_StartVolume)) {
      m_Volume = m_TargetVolume;
      m_FadeTime = 0.0f;
    } else {
      m_Volume += step;
    }
  }

  if (m_Engine) {
    for (auto it = m_Handles.begin(); it != m_Handles.end();) {
      if (m_Engine->isValidVoiceHandle(*it)) {
        m_Engine->setVolume(*it, m_Volume);
        ++it;
      } else {
        it = m_Handles.erase(it);
      }
    }
  }
}

void Bus::SetVolume(float v) {
  m_Volume = v;
  m_TargetVolume = v;
  m_FadeTime = 0.0f;
}

void Bus::SetTargetVolume(float v, float fadeSeconds) {
  m_StartVolume = m_Volume;
  m_TargetVolume = v;
  m_FadeTime = fadeSeconds > 0.0f ? fadeSeconds : 0.001f;
}

float Bus::GetVolume() const { return m_Volume; }
float Bus::GetTargetVolume() const { return m_TargetVolume; }
const std::string &Bus::GetName() const { return m_Name; }

SoLoud::Bus *Bus::Raw() { return m_Bus.get(); }

} // namespace Orpheus
