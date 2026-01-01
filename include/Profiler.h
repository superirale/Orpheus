/**
 * @file Profiler.h
 * @brief Simple profiling utility for performance measurement.
 *
 * Provides a basic Profiler class for measuring execution time
 * of audio operations, and AudioStats for engine statistics.
 */
#pragma once

#include <chrono>
#include <cstdint>

namespace Orpheus {

/**
 * @brief Audio engine statistics snapshot.
 *
 * Contains real-time information about the audio engine state.
 */
struct AudioStats {
  uint32_t activeVoices = 0;  ///< Currently playing (real) voices
  uint32_t virtualVoices = 0; ///< Virtualized voices
  uint32_t totalVoices = 0;   ///< Total tracked voices
  uint32_t maxVoices = 0;     ///< Maximum voice limit
  float cpuUsage = 0.0f;      ///< Estimated CPU usage (0-100%)
  size_t memoryUsed = 0;      ///< Estimated memory usage in bytes
  uint32_t sampleRate = 0;    ///< Engine sample rate
  uint32_t bufferSize = 0;    ///< Buffer size in samples
  uint32_t channels = 0;      ///< Number of output channels
};

/**
 * @brief Simple profiler for measuring execution time.
 *
 * Used to measure performance of audio processing operations.
 *
 * @par Example Usage:
 * @code
 * Profiler prof;
 * prof.Start();
 * // ... expensive operation ...
 * prof.Stop();
 * prof.Print(); // Outputs elapsed time
 * @endcode
 */
class Profiler {
public:
  /**
   * @brief Start the profiler timer.
   */
  void Start();

  /**
   * @brief Stop the profiler timer.
   */
  void Stop();

  /**
   * @brief Print the elapsed time to stdout.
   */
  void Print();

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_StopTime;
};

} // namespace Orpheus
