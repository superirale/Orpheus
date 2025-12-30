#pragma once

class Bus {
public:
  Bus(const std::string &name) : m_Name(name) {
    m_Bus.reset(new SoLoud::Bus());
    // In SoLoud, Bus is a Sound with its own handle; we'll attach filters
    // directly
  }

  // Attach a SoLoud filter
  void addFilter(std::shared_ptr<SoLoud::Filter> f) {
    // SoLoud::Bus doesn't have direct addFilter in all versions; commonly
    // filters are added via handle
    filters.push_back(f);
  }

  void update(float dt) {
    // simple linear interpolation toward target
    float speed = 3.0f; // snap speed
    m_Volume += (m_TargetVolume - m_Volume) * dt * speed;
  }

  void setVolume(float v) { m_Volume = v; }
  void setTargetVolume(float v) { m_TargetVolume = v; }
  float getVolume() const { return m_Volume; }

  const std::string &getName() const { return m_Name; }

  // Expose the SoLoud::Bus pointer for routing
  SoLoud::Bus *raw() { return m_Bus.get(); }

private:
  std::unique_ptr<SoLoud::Bus> m_Bus;
  std::string m_Name;
  float m_Volume = 1.0f;
  float m_TargetVolume = 1.0f;
  std::vector<std::shared_ptr<SoLoud::Filter>> filters;
};
