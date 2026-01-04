#include "../include/MusicManager.h"
#include "../include/SoundBank.h"

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include <cmath>
#include <memory>
#include <vector>

namespace Orpheus {

// PIMPL implementation struct
struct MusicManagerImpl {
  SoLoud::Soloud *engine = nullptr;
  SoundBank *bank = nullptr;

  // Timing
  float bpm = 120.0f;
  int beatsPerBar = 4;
  float beatPosition = 0.0f;
  int lastBeat = -1;

  // Current playback
  AudioHandle currentHandle = 0;
  std::string currentSegment;

  // Crossfade
  AudioHandle fadingOutHandle = 0;
  float fadeProgress = 0.0f;
  float fadeDuration = 0.0f;
  float currentVolume = 1.0f;

  // Queue
  std::queue<QueuedSegment> queue;

  // Callbacks
  std::function<void(int)> beatCallback;

  // Store active sounds
  std::vector<std::shared_ptr<SoLoud::AudioSource>> activeSounds;

  MusicManagerImpl(SoLoud::Soloud *eng, SoundBank &bk)
      : engine(eng), bank(&bk) {}
};

MusicManager::MusicManager(NativeEngineHandle engine, SoundBank &bank)
    : m_Impl(std::make_unique<MusicManagerImpl>(
          static_cast<SoLoud::Soloud *>(engine.ptr), bank)) {}

MusicManager::~MusicManager() { Stop(0.0f); }

MusicManager::MusicManager(MusicManager &&) noexcept = default;
MusicManager &MusicManager::operator=(MusicManager &&) noexcept = default;

void MusicManager::SetBPM(float bpm) { m_Impl->bpm = std::max(1.0f, bpm); }

float MusicManager::GetBPM() const { return m_Impl->bpm; }

void MusicManager::SetBeatsPerBar(int beats) { m_Impl->beatsPerBar = beats; }

void MusicManager::PlaySegment(const std::string &segment, float fadeTime) {
  if (m_Impl->currentHandle != 0 && fadeTime > 0.0f) {
    // Move current to fading out
    m_Impl->fadingOutHandle = m_Impl->currentHandle;
    m_Impl->fadeDuration = fadeTime;
    m_Impl->fadeProgress = 0.0f;

    // Start new segment
    auto eventResult = m_Impl->bank->FindEvent(segment);
    if (eventResult.IsError()) {
      return;
    }
    const auto &ed = eventResult.Value();

    auto wavstream = std::make_shared<SoLoud::WavStream>();
    wavstream->load(ed.path.c_str());
    wavstream->setLooping(true);
    m_Impl->currentHandle = m_Impl->engine->play(*wavstream);
    m_Impl->engine->setVolume(m_Impl->currentHandle, 0.0f); // Start silent
    m_Impl->activeSounds.push_back(wavstream);
    m_Impl->currentSegment = segment;
  } else {
    // Stop current if playing
    if (m_Impl->currentHandle != 0) {
      m_Impl->engine->stop(m_Impl->currentHandle);
    }

    // Start new segment
    auto eventResult = m_Impl->bank->FindEvent(segment);
    if (eventResult.IsError()) {
      return;
    }
    const auto &ed = eventResult.Value();

    auto wavstream = std::make_shared<SoLoud::WavStream>();
    wavstream->load(ed.path.c_str());
    wavstream->setLooping(true);
    m_Impl->currentHandle = m_Impl->engine->play(*wavstream);
    m_Impl->activeSounds.push_back(wavstream);
    m_Impl->currentSegment = segment;
    m_Impl->currentVolume = 1.0f;
  }
}

void MusicManager::QueueSegment(const std::string &segment, TransitionSync sync,
                                float fadeTime) {
  QueuedSegment qs;
  qs.name = segment;
  qs.sync = sync;
  qs.fadeTime = fadeTime;
  m_Impl->queue.push(qs);
}

void MusicManager::PlayStinger(const std::string &stinger, float volume) {
  auto eventResult = m_Impl->bank->FindEvent(stinger);
  if (eventResult.IsError()) {
    return;
  }
  const auto &ed = eventResult.Value();

  auto wav = std::make_shared<SoLoud::Wav>();
  wav->load(ed.path.c_str());
  AudioHandle h = m_Impl->engine->play(*wav);
  m_Impl->engine->setVolume(h, volume);
  m_Impl->activeSounds.push_back(wav);
}

void MusicManager::Stop(float fadeTime) {
  if (m_Impl->currentHandle == 0) {
    return;
  }

  if (fadeTime > 0.0f) {
    m_Impl->engine->fadeVolume(m_Impl->currentHandle, 0.0f, fadeTime);
    m_Impl->engine->scheduleStop(m_Impl->currentHandle, fadeTime);
  } else {
    m_Impl->engine->stop(m_Impl->currentHandle);
  }
  m_Impl->currentHandle = 0;
  m_Impl->currentSegment.clear();
}

void MusicManager::Update(float dt) {
  // Update beat position
  float beatsPerSecond = m_Impl->bpm / 60.0f;
  m_Impl->beatPosition += dt * beatsPerSecond;

  // Check for beat callback
  int currentBeat = static_cast<int>(m_Impl->beatPosition);
  if (currentBeat != m_Impl->lastBeat && m_Impl->beatCallback) {
    m_Impl->beatCallback(currentBeat);
  }
  m_Impl->lastBeat = currentBeat;

  // Process crossfade
  if (m_Impl->fadingOutHandle != 0 && m_Impl->fadeDuration > 0.0f) {
    m_Impl->fadeProgress += dt;
    float t = std::min(m_Impl->fadeProgress / m_Impl->fadeDuration, 1.0f);

    // Fade out old
    m_Impl->engine->setVolume(m_Impl->fadingOutHandle,
                              (1.0f - t) * m_Impl->currentVolume);
    // Fade in new
    if (m_Impl->currentHandle != 0) {
      m_Impl->engine->setVolume(m_Impl->currentHandle,
                                t * m_Impl->currentVolume);
    }

    // Complete crossfade
    if (t >= 1.0f) {
      m_Impl->engine->stop(m_Impl->fadingOutHandle);
      m_Impl->fadingOutHandle = 0;
      m_Impl->fadeProgress = 0.0f;
      m_Impl->fadeDuration = 0.0f;
    }
  }

  // Process queue
  if (!m_Impl->queue.empty()) {
    const auto &next = m_Impl->queue.front();
    bool shouldTransition = false;

    switch (next.sync) {
    case TransitionSync::Immediate:
      shouldTransition = true;
      break;

    case TransitionSync::NextBeat: {
      int curBeat = static_cast<int>(m_Impl->beatPosition);
      if (curBeat != m_Impl->lastBeat) {
        shouldTransition = true;
      }
      break;
    }

    case TransitionSync::NextBar: {
      int currentBar =
          static_cast<int>(m_Impl->beatPosition) / m_Impl->beatsPerBar;
      int lastBar = m_Impl->lastBeat / m_Impl->beatsPerBar;
      if (currentBar != lastBar) {
        shouldTransition = true;
      }
      break;
    }
    }

    if (shouldTransition) {
      PlaySegment(next.name, next.fadeTime);
      m_Impl->queue.pop();
    }
  }
}

float MusicManager::GetBeatPosition() const { return m_Impl->beatPosition; }

int MusicManager::GetBarPosition() const {
  return static_cast<int>(m_Impl->beatPosition) / m_Impl->beatsPerBar;
}

bool MusicManager::IsPlaying() const { return m_Impl->currentHandle != 0; }

void MusicManager::SetBeatCallback(std::function<void(int beat)> callback) {
  m_Impl->beatCallback = std::move(callback);
}

} // namespace Orpheus
