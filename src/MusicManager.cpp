#include "../include/MusicManager.h"
#include "../include/SoundBank.h"

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include <cmath>
#include <memory>
#include <vector>

namespace Orpheus {

// Store active stingers and sounds
static std::vector<std::shared_ptr<SoLoud::AudioSource>> s_ActiveSounds;

MusicManager::MusicManager(SoLoud::Soloud &engine, SoundBank &bank)
    : m_Engine(engine), m_Bank(bank) {}

MusicManager::~MusicManager() { Stop(0.0f); }

void MusicManager::SetBPM(float bpm) { m_BPM = std::max(1.0f, bpm); }

void MusicManager::PlaySegment(const std::string &segment, float fadeTime) {
  if (m_CurrentHandle != 0 && fadeTime > 0.0f) {
    StartCrossfade(segment, fadeTime);
  } else {
    // Stop current if playing
    if (m_CurrentHandle != 0) {
      m_Engine.stop(m_CurrentHandle);
    }

    // Start new segment
    auto eventResult = m_Bank.FindEvent(segment);
    if (eventResult.IsError()) {
      return;
    }
    const auto &ed = eventResult.Value();

    auto wavstream = std::make_shared<SoLoud::WavStream>();
    wavstream->load(ed.path.c_str());
    wavstream->setLooping(true);
    m_CurrentHandle = m_Engine.play(*wavstream);
    s_ActiveSounds.push_back(wavstream);
    m_CurrentSegment = segment;
    m_CurrentVolume = 1.0f;
  }
}

void MusicManager::QueueSegment(const std::string &segment, TransitionSync sync,
                                float fadeTime) {
  QueuedSegment qs;
  qs.name = segment;
  qs.sync = sync;
  qs.fadeTime = fadeTime;
  m_Queue.push(qs);
}

void MusicManager::PlayStinger(const std::string &stinger, float volume) {
  auto eventResult = m_Bank.FindEvent(stinger);
  if (eventResult.IsError()) {
    return;
  }
  const auto &ed = eventResult.Value();

  auto wav = std::make_shared<SoLoud::Wav>();
  wav->load(ed.path.c_str());
  AudioHandle h = m_Engine.play(*wav);
  m_Engine.setVolume(h, volume);
  s_ActiveSounds.push_back(wav);
}

void MusicManager::Stop(float fadeTime) {
  if (m_CurrentHandle == 0) {
    return;
  }

  if (fadeTime > 0.0f) {
    m_Engine.fadeVolume(m_CurrentHandle, 0.0f, fadeTime);
    m_Engine.scheduleStop(m_CurrentHandle, fadeTime);
  } else {
    m_Engine.stop(m_CurrentHandle);
  }
  m_CurrentHandle = 0;
  m_CurrentSegment.clear();
}

void MusicManager::Update(float dt) {
  // Update beat position
  float beatsPerSecond = m_BPM / 60.0f;
  m_BeatPosition += dt * beatsPerSecond;

  // Check for beat callback
  int currentBeat = static_cast<int>(m_BeatPosition);
  if (currentBeat != m_LastBeat && m_BeatCallback) {
    m_BeatCallback(currentBeat);
  }
  m_LastBeat = currentBeat;

  // Process crossfade
  if (m_FadingOutHandle != 0 && m_FadeDuration > 0.0f) {
    m_FadeProgress += dt;
    float t = std::min(m_FadeProgress / m_FadeDuration, 1.0f);

    // Fade out old
    m_Engine.setVolume(m_FadingOutHandle, (1.0f - t) * m_CurrentVolume);
    // Fade in new
    if (m_CurrentHandle != 0) {
      m_Engine.setVolume(m_CurrentHandle, t * m_CurrentVolume);
    }

    // Complete crossfade
    if (t >= 1.0f) {
      m_Engine.stop(m_FadingOutHandle);
      m_FadingOutHandle = 0;
      m_FadeProgress = 0.0f;
      m_FadeDuration = 0.0f;
    }
  }

  // Process queue
  ProcessQueue();
}

void MusicManager::StartCrossfade(const std::string &newSegment,
                                  float fadeTime) {
  // Move current to fading out
  m_FadingOutHandle = m_CurrentHandle;
  m_FadeDuration = fadeTime;
  m_FadeProgress = 0.0f;

  // Start new segment
  auto eventResult = m_Bank.FindEvent(newSegment);
  if (eventResult.IsError()) {
    return;
  }
  const auto &ed = eventResult.Value();

  auto wavstream = std::make_shared<SoLoud::WavStream>();
  wavstream->load(ed.path.c_str());
  wavstream->setLooping(true);
  m_CurrentHandle = m_Engine.play(*wavstream);
  m_Engine.setVolume(m_CurrentHandle, 0.0f); // Start silent
  s_ActiveSounds.push_back(wavstream);
  m_CurrentSegment = newSegment;
}

void MusicManager::ProcessQueue() {
  if (m_Queue.empty()) {
    return;
  }

  const auto &next = m_Queue.front();
  bool shouldTransition = false;

  switch (next.sync) {
  case TransitionSync::Immediate:
    shouldTransition = true;
    break;

  case TransitionSync::NextBeat: {
    int currentBeat = static_cast<int>(m_BeatPosition);
    if (currentBeat != m_LastBeat) {
      shouldTransition = true;
    }
    break;
  }

  case TransitionSync::NextBar: {
    int currentBar = static_cast<int>(m_BeatPosition) / m_BeatsPerBar;
    int lastBar = m_LastBeat / m_BeatsPerBar;
    if (currentBar != lastBar) {
      shouldTransition = true;
    }
    break;
  }
  }

  if (shouldTransition) {
    PlaySegment(next.name, next.fadeTime);
    m_Queue.pop();
  }
}

} // namespace Orpheus
