// ReverbBus.h
#pragma once

#include "Types.h"
#include <soloud_freeverbfilter.h>

enum class ReverbPreset { Room, Hall, Cave, Cathedral, Underwater };

class ReverbBus {
public:
  ReverbBus(const std::string &name);

  bool Init(SoLoud::Soloud &engine);
  void ApplyPreset(ReverbPreset preset);
  void SetParams(float wet, float roomSize, float damp, float width);

  void SetWet(float wet, float fadeTime = 0.0f);
  void SetRoomSize(float roomSize, float fadeTime = 0.0f);
  void SetDamp(float damp, float fadeTime = 0.0f);
  void SetWidth(float width, float fadeTime = 0.0f);
  void SetFreeze(bool freeze);

  float GetWet() const;
  float GetRoomSize() const;
  float GetDamp() const;
  float GetWidth() const;
  bool IsFreeze() const;
  bool IsActive() const;
  const std::string &GetName() const;

  SoLoud::Bus &GetBus();
  SoLoud::handle GetBusHandle() const;

  void SendToReverb(SoLoud::Soloud &engine, SoLoud::handle audioHandle,
                    float sendLevel);

private:
  std::string m_Name;
  SoLoud::Bus m_Bus;
  SoLoud::FreeverbFilter m_Reverb;
  SoLoud::Soloud *m_Engine = nullptr;
  SoLoud::handle m_BusHandle = 0;

  float m_Wet = 0.5f;
  float m_RoomSize = 0.5f;
  float m_Damp = 0.5f;
  float m_Width = 1.0f;
  bool m_Freeze = false;

  bool m_Active = false;
};
