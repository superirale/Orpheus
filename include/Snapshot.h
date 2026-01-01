/**
 * @file Snapshot.h
 * @brief Audio mix snapshots for state-based mixing.
 *
 * Provides Snapshot class for storing and applying bus and reverb
 * parameter presets.
 */
#pragma once

#include <string>
#include <unordered_map>

namespace Orpheus {

/**
 * @brief State of a bus within a snapshot.
 */
struct BusState {
  float volume = 1.0f; ///< Bus volume level
};

/**
 * @brief State of a reverb bus within a snapshot.
 */
struct ReverbBusState {
  float wet = 0.5f;      ///< Wet/dry mix
  float roomSize = 0.5f; ///< Room size
  float damp = 0.5f;     ///< High frequency damping
  float width = 1.0f;    ///< Stereo width
};

/**
 * @brief Mix snapshot for storing audio parameter presets.
 *
 * Snapshots store a collection of bus volumes and reverb parameters
 * that can be applied together to transition between mix states
 * (e.g., normal gameplay vs. pause menu, indoor vs. outdoor).
 *
 * @par Example Usage:
 * @code
 * // Create a "Paused" snapshot with quieter music
 * audio.CreateSnapshot("Paused");
 * audio.SetSnapshotBusVolume("Paused", "Music", 0.3f);
 * audio.SetSnapshotBusVolume("Paused", "SFX", 0.5f);
 *
 * // Apply when pausing
 * audio.ApplySnapshot("Paused", 0.5f); // 0.5s fade
 * @endcode
 */
class Snapshot {
public:
  /**
   * @brief Set a bus state in this snapshot.
   * @param busName Name of the bus.
   * @param state Bus state to store.
   */
  void SetBusState(const std::string &busName, const BusState &state) {
    busStates[busName] = state;
  }

  /**
   * @brief Get all bus states in this snapshot.
   * @return Const reference to the bus state map.
   */
  const std::unordered_map<std::string, BusState> &GetStates() const {
    return busStates;
  }

  /**
   * @brief Set a reverb bus state in this snapshot.
   * @param reverbBusName Name of the reverb bus.
   * @param state Reverb state to store.
   */
  void SetReverbState(const std::string &reverbBusName,
                      const ReverbBusState &state) {
    reverbStates[reverbBusName] = state;
  }

  /**
   * @brief Get all reverb states in this snapshot.
   * @return Const reference to the reverb state map.
   */
  const std::unordered_map<std::string, ReverbBusState> &
  GetReverbStates() const {
    return reverbStates;
  }

  /**
   * @brief Check if this snapshot has state for a reverb bus.
   * @param reverbBusName Name of the reverb bus.
   * @return true if state exists for the bus.
   */
  bool HasReverbState(const std::string &reverbBusName) const {
    return reverbStates.count(reverbBusName) > 0;
  }

private:
  std::unordered_map<std::string, BusState> busStates;
  std::unordered_map<std::string, ReverbBusState> reverbStates;
};

} // namespace Orpheus
