// AudioZone.h
#pragma once
#include "Types.h"

class AudioZone {
public:
  AudioZone(SoLoud::Soloud &engine, const std::string &soundPath,
            const Vector3 &position, float innerRadius, float outerRadius,
            bool stream = true)
      : m_Engine(engine), m_SoundPath(soundPath), m_Position(position),
        m_InnerRadius(innerRadius), m_OuterRadius(outerRadius),
        m_Stream(stream) {
    if (m_Stream) {
      auto wavStream = std::make_shared<SoLoud::WavStream>();
      wavStream->load(soundPath.c_str());
      m_Source = wavStream;
    } else {
      auto wav = std::make_shared<SoLoud::Wav>();
      wav->load(soundPath.c_str());
      m_Source = wav;
    }
    m_Source->setLooping(true);
    mHandle = 0;
  }

  void update(const Vector3 &listenerPos) {
    float dist = distance(listenerPos, m_Position);
    float vol = computeVolume(dist);

    if (vol > 0.0f) {
      // If not playing yet, start
      if (mHandle == 0 || !m_Engine.isValidVoiceHandle(mHandle)) {
        mHandle = m_Engine.play(*m_Source, vol);
        m_Engine.set3dSourceParameters(mHandle, m_Position.x, m_Position.y,
                                       m_Position.z);
      } else {
        m_Engine.setVolume(mHandle, vol);
      }
    } else {
      // Stop when too far
      if (mHandle && m_Engine.isValidVoiceHandle(mHandle))
        m_Engine.stop(mHandle);
    }
  }

private:
  float distance(const Vector3 &a, const Vector3 &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
  }

  float computeVolume(float dist) {
    if (dist < m_InnerRadius)
      return 1.0f;
    if (dist > m_OuterRadius)
      return 0.0f;
    return 1.0f - ((dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius));
  }

  SoLoud::Soloud &m_Engine;
  std::shared_ptr<SoLoud::AudioSource> m_Source;
  std::string m_SoundPath;
  Vector3 m_Position;
  float m_InnerRadius;
  float m_OuterRadius;
  bool m_Stream;
  SoLoud::handle mHandle;
};
