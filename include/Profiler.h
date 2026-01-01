/**
 * @file Profiler.h
 * @brief Simple profiling utility for performance measurement.
 *
 * Provides a basic Profiler class for measuring execution time
 * of audio operations.
 */
#pragma once

#include <chrono>

namespace Orpheus {

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
