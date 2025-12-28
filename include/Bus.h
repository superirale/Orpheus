#pragma once

class Bus {
public:
  Bus(SoLoud::Soloud &engine, const std::string &name)
      : m_Engine(engine), mName(name) {
    mBus.reset(new SoLoud::Bus());
    // In SoLoud, Bus is a Sound with its own handle; we'll attach filters
    // directly
  }

  // Attach a SoLoud filter
  void addFilter(std::shared_ptr<SoLoud::Filter> f) {
    // SoLoud::Bus doesn't have direct addFilter in all versions; commonly
    // filters are added via handle
    filters.push_back(f);
  }

  void setVolume(float v) { mVolume = v; }
  float getVolume() const { return mVolume; }

  const std::string &getName() const { return mName; }

  // Expose the SoLoud::Bus pointer for routing
  SoLoud::Bus *raw() { return mBus.get(); }

private:
  SoLoud::Soloud &m_Engine;
  std::unique_ptr<SoLoud::Bus> mBus;
  std::string mName;
  float mVolume = 1.0f;
  std::vector<std::shared_ptr<SoLoud::Filter>> filters;
};
