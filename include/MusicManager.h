/**
 * @file MusicManager.h
 * @brief Interactive music system for beat-synced playback.
 *
 * Provides stingers, transitions, and beat-synchronized segment changes.
 */
#pragma once

#include <functional>
#include <queue>
#include <string>

#include "Types.h"

namespace SoLoud {
class Soloud;
}

namespace Orpheus {

class SoundBank;

/**
 * @brief Transition sync point for queued segments.
 */
enum class TransitionSync {
  Immediate, ///< Transition immediately
  NextBeat,  ///< Wait for next beat
  NextBar    ///< Wait for next bar (default 4 beats)
};

/**
 * @brief Queued music segment.
 */
struct QueuedSegment {
  std::string name;
  TransitionSync sync = TransitionSync::NextBeat;
  float fadeTime = 0.5f;
};

/**
 * @brief Interactive music manager.
 *
 * Handles beat-synchronized segment changes, stingers, and crossfades.
 */
class MusicManager {
public:
  /**
   * @brief Construct MusicManager.
   * @param engine Reference to SoLoud engine.
   * @param bank Reference to sound bank.
   */
  MusicManager(SoLoud::Soloud &engine, SoundBank &bank);
  ~MusicManager();

  /**
   * @brief Set beats per minute for synchronization.
   * @param bpm Tempo (default: 120).
   */
  void SetBPM(float bpm);

  /**
   * @brief Get current BPM.
   */
  [[nodiscard]] float GetBPM() const { return m_BPM; }

  /**
   * @brief Set beats per bar for bar-sync transitions.
   * @param beats Beats per bar (default: 4).
   */
  void SetBeatsPerBar(int beats) { m_BeatsPerBar = beats; }

  /**
   * @brief Play a music segment immediately.
   * @param segment Event name of segment to play.
   * @param fadeTime Crossfade duration in seconds.
   */
  void PlaySegment(const std::string &segment, float fadeTime = 0.5f);

  /**
   * @brief Queue a segment to play at the next sync point.
   * @param segment Event name of segment to play.
   * @param sync When to transition (Immediate, NextBeat, NextBar).
   * @param fadeTime Crossfade duration in seconds.
   */
  void QueueSegment(const std::string &segment,
                    TransitionSync sync = TransitionSync::NextBeat,
                    float fadeTime = 0.5f);

  /**
   * @brief Play a one-shot stinger over the current music.
   * @param stinger Event name of stinger to play.
   * @param volume Stinger volume (default: 1.0).
   */
  void PlayStinger(const std::string &stinger, float volume = 1.0f);

  /**
   * @brief Stop all music with optional fade.
   * @param fadeTime Fade out duration in seconds.
   */
  void Stop(float fadeTime = 0.5f);

  /**
   * @brief Update music manager (call each frame).
   * @param dt Delta time in seconds.
   */
  void Update(float dt);

  /**
   * @brief Get current beat position (0-based).
   */
  [[nodiscard]] float GetBeatPosition() const { return m_BeatPosition; }

  /**
   * @brief Get current bar position (0-based).
   */
  [[nodiscard]] int GetBarPosition() const {
    return static_cast<int>(m_BeatPosition) / m_BeatsPerBar;
  }

  /**
   * @brief Check if music is currently playing.
   */
  [[nodiscard]] bool IsPlaying() const { return m_CurrentHandle != 0; }

  /**
   * @brief Set callback for beat events.
   */
  void SetBeatCallback(std::function<void(int beat)> callback) {
    m_BeatCallback = std::move(callback);
  }

private:
  void StartCrossfade(const std::string &newSegment, float fadeTime);
  void ProcessQueue();

  SoLoud::Soloud &m_Engine;
  SoundBank &m_Bank;

  // Timing
  float m_BPM = 120.0f;
  int m_BeatsPerBar = 4;
  float m_BeatPosition = 0.0f;
  int m_LastBeat = -1;

  // Current playback
  AudioHandle m_CurrentHandle = 0;
  std::string m_CurrentSegment;

  // Crossfade
  AudioHandle m_FadingOutHandle = 0;
  float m_FadeProgress = 0.0f;
  float m_FadeDuration = 0.0f;
  float m_CurrentVolume = 1.0f;

  // Queue
  std::queue<QueuedSegment> m_Queue;

  // Callbacks
  std::function<void(int)> m_BeatCallback;
};

} // namespace Orpheus
