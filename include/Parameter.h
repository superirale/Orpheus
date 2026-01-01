/**
 * @file Parameter.h
 * @brief Global audio parameters with change notification.
 *
 * Provides a Parameter class for exposing and observing named
 * values that can drive audio behavior.
 */
#pragma once

#include <functional>
#include <vector>

namespace Orpheus {

/**
 * @brief Observable audio parameter.
 *
 * A named float value that notifies registered listeners when changed.
 * Used for things like game time, intensity values, or other dynamic
 * parameters that affect audio behavior.
 *
 * @par Example Usage:
 * @code
 * Parameter intensity(0.5f);
 * intensity.Bind([](float v) { std::cout << "Intensity: " << v << "\n"; });
 * intensity.Set(1.0f); // Triggers callback
 * @endcode
 */
class Parameter {
public:
  /**
   * @brief Construct a parameter with an initial value.
   * @param v Initial value (default: 0.0f).
   */
  Parameter(float v = 0.0f) : value(v) {}

  /**
   * @brief Get the current value.
   * @return Current parameter value.
   */
  [[nodiscard]] float Get() const { return value; }

  /**
   * @brief Set the parameter value and notify listeners.
   * @param v New value.
   */
  void Set(float v) {
    value = v;
    for (auto &cb : listeners)
      cb(value);
  }

  /**
   * @brief Register a callback to be notified when value changes.
   * @param cb Callback function receiving the new value.
   */
  void Bind(std::function<void(float)> cb) { listeners.push_back(cb); }

private:
  float value;
  std::vector<std::function<void(float)>> listeners;
};

} // namespace Orpheus
