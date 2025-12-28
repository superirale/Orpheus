#pragma once

class Parameter {
public:
  Parameter(float v = 0.0f) : value(v) {}
  float Get() const { return value; }
  void Set(float v) {
    value = v;
    for (auto &cb : listeners)
      cb(value);
  }
  void Bind(std::function<void(float)> cb) { listeners.push_back(cb); }

private:
  float value;
  std::vector<std::function<void(float)>> listeners;
};
