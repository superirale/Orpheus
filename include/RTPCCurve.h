/**
 * @file RTPCCurve.h
 * @brief Real-Time Parameter Control curves for audio automation.
 *
 * Maps parameter values to effect outputs through interpolated control points.
 */
#pragma once

#include <algorithm>
#include <functional>
#include <vector>

namespace Orpheus {

/**
 * @brief A point on an RTPC curve.
 */
struct CurvePoint {
  float x = 0.0f; ///< Input value (normalized 0-1 or custom range)
  float y = 0.0f; ///< Output value

  bool operator<(const CurvePoint &other) const { return x < other.x; }
};

/**
 * @brief RTPC curve for mapping parameter values to effect outputs.
 *
 * Provides linear interpolation between control points to create
 * smooth parameter-to-effect mappings.
 *
 * @par Example Usage:
 * @code
 * RTPCCurve enginePitch;
 * enginePitch.AddPoint(0.0f, 0.8f);   // Idle: low pitch
 * enginePitch.AddPoint(0.5f, 1.2f);   // Mid: normal pitch
 * enginePitch.AddPoint(1.0f, 2.0f);   // Max: high pitch
 *
 * float pitch = enginePitch.Evaluate(0.75f); // Returns ~1.6
 * @endcode
 */
class RTPCCurve {
public:
  /**
   * @brief Add a control point to the curve.
   * @param x Input value.
   * @param y Output value.
   */
  void AddPoint(float x, float y) {
    m_Points.push_back({x, y});
    std::sort(m_Points.begin(), m_Points.end());
  }

  /**
   * @brief Clear all control points.
   */
  void Clear() { m_Points.clear(); }

  /**
   * @brief Get the number of control points.
   */
  [[nodiscard]] size_t GetPointCount() const { return m_Points.size(); }

  /**
   * @brief Evaluate the curve at a given input value.
   *
   * Uses linear interpolation between control points.
   * Values outside the curve range clamp to the nearest endpoint.
   *
   * @param input Input value to evaluate.
   * @return Interpolated output value.
   */
  [[nodiscard]] float Evaluate(float input) const {
    if (m_Points.empty()) {
      return 0.0f;
    }

    if (m_Points.size() == 1) {
      return m_Points[0].y;
    }

    // Clamp to curve range
    if (input <= m_Points.front().x) {
      return m_Points.front().y;
    }
    if (input >= m_Points.back().x) {
      return m_Points.back().y;
    }

    // Find surrounding points and interpolate
    for (size_t i = 0; i < m_Points.size() - 1; ++i) {
      if (input >= m_Points[i].x && input <= m_Points[i + 1].x) {
        float range = m_Points[i + 1].x - m_Points[i].x;
        if (range <= 0.0f) {
          return m_Points[i].y;
        }
        float t = (input - m_Points[i].x) / range;
        return m_Points[i].y + t * (m_Points[i + 1].y - m_Points[i].y);
      }
    }

    return m_Points.back().y;
  }

  /**
   * @brief Get a control point by index.
   */
  [[nodiscard]] const CurvePoint &GetPoint(size_t index) const {
    return m_Points[index];
  }

private:
  std::vector<CurvePoint> m_Points;
};

/**
 * @brief Binding between a parameter, curve, and effect callback.
 */
struct RTPCBinding {
  std::string parameterName;
  RTPCCurve curve;
  std::function<void(float)> callback;
};

} // namespace Orpheus
