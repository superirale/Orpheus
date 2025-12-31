
#pragma once

struct BusState {
  float volume = 1.0f;
};

// Reverb bus state for snapshot modulation
struct ReverbBusState {
  float wet = 0.5f;
  float roomSize = 0.5f;
  float damp = 0.5f;
  float width = 1.0f;
};

class Snapshot {
public:
  // Bus volume control
  void SetBusState(const std::string &busName, const BusState &state) {
    busStates[busName] = state;
  }

  const std::unordered_map<std::string, BusState> &GetStates() const {
    return busStates;
  }

  // Reverb bus parameter control
  void SetReverbState(const std::string &reverbBusName,
                      const ReverbBusState &state) {
    reverbStates[reverbBusName] = state;
  }

  const std::unordered_map<std::string, ReverbBusState> &
  GetReverbStates() const {
    return reverbStates;
  }

  bool HasReverbState(const std::string &reverbBusName) const {
    return reverbStates.count(reverbBusName) > 0;
  }

private:
  std::unordered_map<std::string, BusState> busStates;
  std::unordered_map<std::string, ReverbBusState> reverbStates;
};
