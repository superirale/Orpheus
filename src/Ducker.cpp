#include "../include/Ducker.h"
#include "../include/Bus.h"

#include <algorithm>
#include <cmath>

namespace Orpheus {

std::string Ducker::MakeKey(const std::string &target,
                            const std::string &sidechain) {
  return target + ":" + sidechain;
}

void Ducker::AddRule(const DuckingRule &rule) {
  // Check if rule already exists
  for (const auto &existing : m_Rules) {
    if (existing.targetBus == rule.targetBus &&
        existing.sidechainBus == rule.sidechainBus) {
      return; // Already exists
    }
  }

  m_Rules.push_back(rule);
  m_States[MakeKey(rule.targetBus, rule.sidechainBus)] = DuckingState{};
}

void Ducker::RemoveRule(const std::string &targetBus,
                        const std::string &sidechainBus) {
  m_Rules.erase(std::remove_if(m_Rules.begin(), m_Rules.end(),
                               [&](const DuckingRule &r) {
                                 return r.targetBus == targetBus &&
                                        r.sidechainBus == sidechainBus;
                               }),
                m_Rules.end());

  m_States.erase(MakeKey(targetBus, sidechainBus));
}

void Ducker::ClearRules() {
  m_Rules.clear();
  m_States.clear();
}

void Ducker::Update(
    float dt, std::unordered_map<std::string, std::shared_ptr<Bus>> &buses,
    std::function<bool(const std::string &)> getSidechainActive) {

  // Track cumulative duck level per target bus
  std::unordered_map<std::string, float> targetLevels;

  for (auto &rule : m_Rules) {
    std::string key = MakeKey(rule.targetBus, rule.sidechainBus);
    auto &state = m_States[key];

    // Check if sidechain bus has active audio
    bool sidechainActive = getSidechainActive(rule.sidechainBus);

    if (sidechainActive) {
      // Sidechain is active - duck the target
      state.active = true;
      state.holdTimer = rule.holdTime;

      // Attack phase - fade down
      float attackRate = 1.0f / std::max(rule.attackTime, 0.001f);
      state.currentLevel -= attackRate * dt;
      state.currentLevel = std::max(state.currentLevel, rule.duckLevel);
    } else {
      // Sidechain stopped
      if (state.holdTimer > 0.0f) {
        // Still in hold phase
        state.holdTimer -= dt;
      } else {
        // Release phase - fade up
        state.active = false;
        float releaseRate = 1.0f / std::max(rule.releaseTime, 0.001f);
        state.currentLevel += releaseRate * dt;
        state.currentLevel = std::min(state.currentLevel, 1.0f);
      }
    }

    // Accumulate the lowest duck level for this target
    if (targetLevels.find(rule.targetBus) == targetLevels.end()) {
      targetLevels[rule.targetBus] = state.currentLevel;
    } else {
      targetLevels[rule.targetBus] =
          std::min(targetLevels[rule.targetBus], state.currentLevel);
    }
  }

  // Apply duck levels to buses
  for (const auto &[busName, level] : targetLevels) {
    auto it = buses.find(busName);
    if (it != buses.end() && it->second) {
      // Only set volume if level has changed significantly
      float currentVolume = it->second->GetVolume();
      if (std::abs(currentVolume - level) > 0.001f) {
        it->second->SetVolume(level);
      }
    }
  }
}

bool Ducker::IsDucking(const std::string &targetBus) const {
  for (const auto &rule : m_Rules) {
    if (rule.targetBus == targetBus) {
      std::string key = MakeKey(rule.targetBus, rule.sidechainBus);
      auto it = m_States.find(key);
      if (it != m_States.end() && it->second.active) {
        return true;
      }
    }
  }
  return false;
}

float Ducker::GetDuckLevel(const std::string &targetBus) const {
  float minLevel = 1.0f;
  for (const auto &rule : m_Rules) {
    if (rule.targetBus == targetBus) {
      std::string key = MakeKey(rule.targetBus, rule.sidechainBus);
      auto it = m_States.find(key);
      if (it != m_States.end()) {
        minLevel = std::min(minLevel, it->second.currentLevel);
      }
    }
  }
  return minLevel;
}

} // namespace Orpheus
