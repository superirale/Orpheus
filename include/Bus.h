#pragma once

#include <memory>
#include <string>
#include <vector>

#include <soloud.h>
#include <soloud_bus.h>

namespace Orpheus {

class Bus {
public:
  Bus(const std::string &name);

  void AddFilter(std::shared_ptr<SoLoud::Filter> f);
  void AddHandle(SoLoud::Soloud &engine, SoLoud::handle h);
  void Update(float dt);

  void SetVolume(float v);
  void SetTargetVolume(float v, float fadeSeconds = 0.0f);

  float GetVolume() const;
  float GetTargetVolume() const;
  const std::string &GetName() const;

  SoLoud::Bus *Raw();

private:
  std::unique_ptr<SoLoud::Bus> m_Bus;
  std::string m_Name;
  float m_Volume = 1.0f;
  float m_TargetVolume = 1.0f;
  float m_StartVolume = 1.0f;
  float m_FadeTime = 0.0f;
  std::vector<std::shared_ptr<SoLoud::Filter>> filters;
  std::vector<SoLoud::handle> m_Handles;
  SoLoud::Soloud *m_Engine = nullptr;
};

} // namespace Orpheus
