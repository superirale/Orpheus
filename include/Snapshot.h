
#pragma once

struct BusState {
  float volume = 1.0f;
};

class Snapshot {
public:
  void setBusState(const std::string &busName, const BusState &state) {
    busStates[busName] = state;
  }

  const std::unordered_map<std::string, BusState> &getStates() const {
    return busStates;
  }

private:
  std::unordered_map<std::string, BusState> busStates;
};
