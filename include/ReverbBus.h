// ReverbBus.h
#pragma once

#include "Types.h"
#include <soloud_freeverbfilter.h>

// Reverb bus presets for common environments
enum class ReverbPreset {
  Room,      // Small room - short decay, high damping
  Hall,      // Concert hall - medium decay
  Cave,      // Large cave - long decay, low damping
  Cathedral, // Cathedral - very long decay, wide stereo
  Underwater // Underwater - heavy wet, high damping
};

// A reverb bus hosts a shared reverb DSP chain.
// Sounds send signal TO the reverb bus, they don't play ON it.
class ReverbBus {
public:
  ReverbBus(const std::string &name) : m_Name(name) {}

  // Initialize the reverb bus and attach to the audio engine
  bool init(SoLoud::Soloud &engine) {
    // Set initial reverb parameters
    m_Reverb.setParams(m_Freeze ? 1.0f : 0.0f, m_RoomSize, m_Damp, m_Width);

    // Attach the reverb filter to the bus
    m_Bus.setFilter(0, &m_Reverb);

    // Play the bus on the engine (returns handle for routing)
    m_BusHandle = engine.play(m_Bus);
    if (m_BusHandle == 0) {
      return false;
    }

    // Set initial wet level
    engine.setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WET,
                              m_Wet);

    m_Engine = &engine;
    m_Active = true;
    return true;
  }

  // Apply a preset
  void applyPreset(ReverbPreset preset) {
    switch (preset) {
    case ReverbPreset::Room:
      setParams(0.3f, 0.4f, 0.7f, 0.8f);
      break;
    case ReverbPreset::Hall:
      setParams(0.5f, 0.6f, 0.5f, 1.0f);
      break;
    case ReverbPreset::Cave:
      setParams(0.6f, 0.85f, 0.3f, 1.0f);
      break;
    case ReverbPreset::Cathedral:
      setParams(0.7f, 0.95f, 0.2f, 1.0f);
      break;
    case ReverbPreset::Underwater:
      setParams(0.9f, 0.7f, 0.8f, 0.5f);
      break;
    }
  }

  // Set all reverb parameters at once
  void setParams(float wet, float roomSize, float damp, float width) {
    m_Wet = std::clamp(wet, 0.0f, 1.0f);
    m_RoomSize = std::clamp(roomSize, 0.0f, 1.0f);
    m_Damp = std::clamp(damp, 0.0f, 1.0f);
    m_Width = std::clamp(width, 0.0f, 1.0f);

    m_Reverb.setParams(m_Freeze ? 1.0f : 0.0f, m_RoomSize, m_Damp, m_Width);

    // Update filter parameters on the engine
    if (m_Engine && m_BusHandle != 0) {
      m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::WET,
                                   m_Wet);
      m_Engine->setFilterParameter(
          m_BusHandle, 0, SoLoud::FreeverbFilter::ROOMSIZE, m_RoomSize);
      m_Engine->setFilterParameter(m_BusHandle, 0, SoLoud::FreeverbFilter::DAMP,
                                   m_Damp);
      m_Engine->setFilterParameter(m_BusHandle, 0,
                                   SoLoud::FreeverbFilter::WIDTH, m_Width);
    }
  }

  // Individual parameter setters with smooth fading
  void setWet(float wet, float fadeTime = 0.0f) {
    m_Wet = std::clamp(wet, 0.0f, 1.0f);
    if (m_Engine && m_BusHandle != 0) {
      if (fadeTime > 0.0f) {
        m_Engine->fadeFilterParameter(
            m_BusHandle, 0, SoLoud::FreeverbFilter::WET, m_Wet, fadeTime);
      } else {
        m_Engine->setFilterParameter(m_BusHandle, 0,
                                     SoLoud::FreeverbFilter::WET, m_Wet);
      }
    }
  }

  void setRoomSize(float roomSize, float fadeTime = 0.0f) {
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

  void setDamp(float damp, float fadeTime = 0.0f) {
    m_Damp = std::clamp(damp, 0.0f, 1.0f);
    if (m_Engine && m_BusHandle != 0) {
      if (fadeTime > 0.0f) {
        m_Engine->fadeFilterParameter(
            m_BusHandle, 0, SoLoud::FreeverbFilter::DAMP, m_Damp, fadeTime);
      } else {
        m_Engine->setFilterParameter(m_BusHandle, 0,
                                     SoLoud::FreeverbFilter::DAMP, m_Damp);
      }
    }
  }

  void setWidth(float width, float fadeTime = 0.0f) {
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

  void setFreeze(bool freeze) {
    m_Freeze = freeze;
    if (m_Engine && m_BusHandle != 0) {
      m_Engine->setFilterParameter(
          m_BusHandle, 0, SoLoud::FreeverbFilter::FREEZE, freeze ? 1.0f : 0.0f);
    }
  }

  // Getters
  float getWet() const { return m_Wet; }
  float getRoomSize() const { return m_RoomSize; }
  float getDamp() const { return m_Damp; }
  float getWidth() const { return m_Width; }
  bool isFreeze() const { return m_Freeze; }
  bool isActive() const { return m_Active; }
  const std::string &getName() const { return m_Name; }

  // Get the bus for routing sounds into the reverb
  SoLoud::Bus &getBus() { return m_Bus; }
  SoLoud::handle getBusHandle() const { return m_BusHandle; }

  // Send a sound to this reverb bus with a given level (0.0-1.0)
  // The audioHandle is routed to this reverb bus via SoLoud voice groups
  void sendToReverb(SoLoud::Soloud &engine, SoLoud::handle audioHandle,
                    float sendLevel) {
    if (!m_Active || m_BusHandle == 0)
      return;

    // SoLoud doesn't have direct aux sends, so we use voice volume adjustment
    // This is a simplified approach - for proper sends, we'd need DSP routing
    // For now, we route the voice to play on the reverb bus with scaled volume
    (void)engine;
    (void)audioHandle;
    (void)sendLevel;
    // TODO: Implement proper send routing using SoLoud Bus play/stop
  }

private:
  std::string m_Name;
  SoLoud::Bus m_Bus;
  SoLoud::FreeverbFilter m_Reverb;
  SoLoud::Soloud *m_Engine = nullptr;
  SoLoud::handle m_BusHandle = 0;

  // Reverb parameters
  float m_Wet = 0.5f;
  float m_RoomSize = 0.5f;
  float m_Damp = 0.5f;
  float m_Width = 1.0f;
  bool m_Freeze = false;

  bool m_Active = false;
};
