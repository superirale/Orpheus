/**
 * @file Ducker.h
 * @brief Sidechaining/ducking for automatic volume control.
 *
 * Provides the Ducker class for reducing volume on target buses
 * when audio is playing on sidechain buses (e.g., music ducks during dialogue).
 */
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Orpheus {

class Bus;

/**
 * @brief Configuration for a ducking rule.
 *
 * Defines which bus to duck when audio is detected on the sidechain bus.
 */
struct DuckingRule {
  std::string targetBus;    ///< Bus to duck (e.g., "Music")
  std::string sidechainBus; ///< Bus that triggers ducking (e.g., "Dialogue")
  float duckLevel = 0.3f;   ///< Target volume when ducked (0.0-1.0)
  float attackTime = 0.1f;  ///< Fade down time in seconds
  float releaseTime = 0.5f; ///< Fade up time in seconds
  float holdTime = 0.1f;    ///< Hold ducked level after sidechain stops
};

/**
 * @brief State of a single ducking rule at runtime.
 */
struct DuckingState {
  bool active = false;       ///< Is the sidechain bus currently active
  float currentLevel = 1.0f; ///< Current volume multiplier
  float holdTimer = 0.0f;    ///< Time remaining in hold phase
};

/**
 * @brief Manages automatic volume ducking between buses.
 *
 * The Ducker monitors voice activity on sidechain buses and fades
 * the volume of target buses accordingly.
 *
 * @par Example Usage:
 * @code
 * // Duck music when dialogue plays
 * audio.AddDuckingRule({
 *   .targetBus = "Music",
 *   .sidechainBus = "Dialogue",
 *   .duckLevel = 0.3f,
 *   .attackTime = 0.1f,
 *   .releaseTime = 0.5f
 * });
 * @endcode
 */
class Ducker {
public:
  /**
   * @brief Add a ducking rule.
   * @param rule The ducking configuration.
   */
  void AddRule(const DuckingRule &rule);

  /**
   * @brief Remove a ducking rule.
   * @param targetBus Target bus name.
   * @param sidechainBus Sidechain bus name.
   */
  void RemoveRule(const std::string &targetBus,
                  const std::string &sidechainBus);

  /**
   * @brief Clear all ducking rules.
   */
  void ClearRules();

  /**
   * @brief Update ducking state and apply volume changes.
   * @param dt Delta time in seconds.
   * @param buses Map of bus names to Bus pointers.
   * @param getSidechainActive Function to check if sidechain bus has active
   * voices.
   */
  void Update(float dt,
              std::unordered_map<std::string, std::shared_ptr<Bus>> &buses,
              std::function<bool(const std::string &)> getSidechainActive);

  /**
   * @brief Check if a target bus is currently being ducked.
   * @param targetBus Bus name to check.
   * @return true if the bus is being ducked.
   */
  [[nodiscard]] bool IsDucking(const std::string &targetBus) const;

  /**
   * @brief Get the current duck level for a target bus.
   * @param targetBus Bus name.
   * @return Current volume multiplier (1.0 = not ducked).
   */
  [[nodiscard]] float GetDuckLevel(const std::string &targetBus) const;

private:
  std::vector<DuckingRule> m_Rules;
  std::unordered_map<std::string, DuckingState>
      m_States; // Key: "target:sidechain"

  static std::string MakeKey(const std::string &target,
                             const std::string &sidechain);
};

} // namespace Orpheus
