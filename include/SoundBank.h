/**
 * @file SoundBank.h
 * @brief Sound bank for managing audio event definitions.
 *
 * Provides event registration and lookup from JSON definitions.
 */
#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "Error.h"

namespace Orpheus {

/**
 * @brief Playlist playback mode for multi-sound events.
 */
enum class PlaylistMode {
  Single,     ///< Only plays 'path' (default single sound)
  Sequential, ///< Play sounds in order, then loop
  Shuffle,    ///< Randomize order, play each once, then reshuffle
  Random      ///< Pick a random sound each time
};

/**
 * @brief Descriptor for an audio event.
 *
 * Contains all parameters needed to play an audio event including
 * file path, bus routing, volume/pitch randomization, and 3D settings.
 */
struct EventDescriptor {
  std::string name;           ///< Unique event name
  std::string path;           ///< Path to audio file (for single-sound events)
  std::string bus;            ///< Target bus name
  float volumeMin = 1.0f;     ///< Minimum volume (randomization)
  float volumeMax = 1.0f;     ///< Maximum volume (randomization)
  float pitchMin = 1.0f;      ///< Minimum pitch (randomization)
  float pitchMax = 1.0f;      ///< Maximum pitch (randomization)
  bool stream = false;        ///< Use streaming (for long files)
  uint8_t priority = 128;     ///< Voice priority (0-255)
  float maxDistance = 100.0f; ///< Maximum audible distance
  std::unordered_map<std::string, std::string>
      parameters; ///< Custom parameters

  // Playlist fields
  std::vector<std::string> sounds; ///< Multiple sound paths for playlists
  PlaylistMode playlistMode = PlaylistMode::Single; ///< Playlist playback mode
  bool loopPlaylist = false; ///< Loop the playlist after finishing (Sequential)
                             ///< or repeat (Random)
  float interval = 0.0f;     ///< Delay between playlist items (seconds)
  float startDelay = 0.0f;   ///< Initial delay before starting (seconds)
};

/**
 * @brief Manages audio event definitions.
 *
 * SoundBank stores EventDescriptor objects for lookup during playback.
 * Events can be registered programmatically or loaded from JSON files.
 *
 * @par JSON Format:
 * @code{.json}
 * {
 *   "events": [
 *     {
 *       "name": "footstep",
 *       "path": "audio/footstep.wav",
 *       "bus": "SFX",
 *       "volumeMin": 0.8,
 *       "volumeMax": 1.0,
 *       "pitchMin": 0.9,
 *       "pitchMax": 1.1
 *     }
 *   ]
 * }
 * @endcode
 */
class SoundBank {
public:
  /**
   * @brief Load events from a JSON file.
   * @param jsonPath Path to the JSON file.
   * @return Status indicating success or error.
   */
  Status LoadFromJsonFile(const std::string &jsonPath);

  /**
   * @brief Register an event from a JSON string.
   * @param jsonString JSON string containing event definition.
   * @return Status indicating success or error.
   */
  Status RegisterEventFromJson(const std::string &jsonString);

  /**
   * @brief Register an event descriptor directly.
   * @param ed The event descriptor to register.
   */
  void RegisterEvent(const EventDescriptor &ed);

  /**
   * @brief Find an event by name.
   * @param name The event name to look up.
   * @return Result containing the EventDescriptor or an error.
   */
  [[nodiscard]] Result<EventDescriptor> FindEvent(const std::string &name);

private:
  std::unordered_map<std::string, EventDescriptor> events;
};

} // namespace Orpheus
