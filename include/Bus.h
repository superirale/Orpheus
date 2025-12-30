#pragma once

class Bus {
public:
  Bus(const std::string &name) : m_Name(name) {
    m_Bus.reset(new SoLoud::Bus());
  }

  // Attach a SoLoud filter
  void addFilter(std::shared_ptr<SoLoud::Filter> f) { filters.push_back(f); }

  // Add a sound handle to this bus for volume control
  void addHandle(SoLoud::Soloud &engine, SoLoud::handle h) {
    m_Engine = &engine;
    m_Handles.push_back(h);
  }

  void update(float dt) {
    // Calculate interpolation based on fade time
    if (m_FadeTime > 0.0f) {
      // Linear fade over the specified time
      float step = (m_TargetVolume - m_StartVolume) * (dt / m_FadeTime);
      if ((m_TargetVolume > m_StartVolume &&
           m_Volume + step >= m_TargetVolume) ||
          (m_TargetVolume < m_StartVolume &&
           m_Volume + step <= m_TargetVolume) ||
          (m_TargetVolume == m_StartVolume)) {
        m_Volume = m_TargetVolume;
        m_FadeTime = 0.0f; // Fade complete
      } else {
        m_Volume += step;
      }
    }

    // Apply volume to all handles on this bus
    if (m_Engine) {
      for (auto it = m_Handles.begin(); it != m_Handles.end();) {
        if (m_Engine->isValidVoiceHandle(*it)) {
          m_Engine->setVolume(*it, m_Volume);
          ++it;
        } else {
          it = m_Handles.erase(it); // Remove finished handles
        }
      }
    }
  }

  // Immediate volume change (no fade)
  void setVolume(float v) {
    m_Volume = v;
    m_TargetVolume = v;
    m_FadeTime = 0.0f;
  }

  // Set target volume with optional fade time (default: instant)
  void setTargetVolume(float v, float fadeSeconds = 0.0f) {
    m_StartVolume = m_Volume;
    m_TargetVolume = v;
    m_FadeTime =
        fadeSeconds > 0.0f ? fadeSeconds : 0.001f; // Small epsilon for instant
  }

  float getVolume() const { return m_Volume; }
  float getTargetVolume() const { return m_TargetVolume; }

  const std::string &getName() const { return m_Name; }

  // Expose the SoLoud::Bus pointer for routing
  SoLoud::Bus *raw() { return m_Bus.get(); }

private:
  std::unique_ptr<SoLoud::Bus> m_Bus;
  std::string m_Name;
  float m_Volume = 1.0f;
  float m_TargetVolume = 1.0f;
  float m_StartVolume = 1.0f;
  float m_FadeTime = 0.0f; // Remaining fade time in seconds
  std::vector<std::shared_ptr<SoLoud::Filter>> filters;
  std::vector<SoLoud::handle> m_Handles;
  SoLoud::Soloud *m_Engine = nullptr;
};
