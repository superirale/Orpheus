#pragma once

#include "SoundBank.h"
#include "Types.h"

class AudioEvent {
public:
  AudioEvent(SoLoud::Soloud &engine, SoundBank &bank)
      : m_Engine(engine), m_Bank(bank) {}

  AudioHandle play(const std::string &eventName,
                   const std::string &busName = "Master") {
    EventDescriptor ed;
    if (!m_Bank.findEvent(eventName, ed)) {
      std::cerr << "Event not found: " << eventName << "\n";
      return 0;
    }

    // Choose random volume/pitch in ranges
    float volume = ed.volumeMin; // expand with RNG
    float pitch = ed.pitchMin;

    // Load asset (stream or static). In a real engine cache these handles.
    if (ed.stream) {
      auto wavstream = std::make_shared<SoLoud::WavStream>();
      wavstream->load(ed.path.c_str());
      // play through the bus: SoLoud::Soloud::play() accepts a sound object
      AudioHandle h = m_Engine.play(*wavstream);
      m_ActiveSounds.push_back(wavstream); // keep alive
      m_Engine.setVolume(h, volume);
      m_Engine.setSamplerate(
          h, (unsigned int)(m_Engine.getBackendSamplerate() * pitch));
      routeHandleToBus(h, busName);
      return h;
    } else {
      auto wav = std::make_shared<SoLoud::Wav>();
      wav->load(ed.path.c_str());
      AudioHandle h = m_Engine.play(*wav);
      m_ActiveSounds.push_back(wav);
      m_Engine.setVolume(h, volume);
      m_Engine.setSamplerate(
          h, (unsigned int)(m_Engine.getBackendSamplerate() * pitch));
      routeHandleToBus(h, busName);
      return h;
    }
  }

private:
  void routeHandleToBus(AudioHandle h, const std::string &busName) {
    // Ideal: SoLoud supports playing via a specific Bus instance. If not
    // available, you can mix by playing the sound but applying filters/volumes
    // based on bus.
    (void)h;
    (void)busName;
  }

  SoLoud::Soloud &m_Engine;
  SoundBank &m_Bank;
  std::vector<std::shared_ptr<SoLoud::AudioSource>>
      m_ActiveSounds; // keep them alive while playing
};
