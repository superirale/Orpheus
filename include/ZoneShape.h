/**
 * @file ZoneShape.h
 * @brief Zone shape definitions for spatial audio regions.
 *
 * Provides different geometric shapes for defining audio zones.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "Types.h"

namespace Orpheus {

/**
 * @brief 2D point for polygon shapes.
 */
struct Vector2 {
  float x = 0.0f;
  float y = 0.0f;
};

/**
 * @brief Type of zone shape.
 */
enum class ZoneShapeType { Sphere, Box, Polygon };

/**
 * @brief Base class for zone shapes.
 */
class ZoneShape {
public:
  virtual ~ZoneShape() = default;

  /**
   * @brief Check if a point is inside the shape.
   * @param point World position to check.
   * @return true if point is inside the shape.
   */
  [[nodiscard]] virtual bool Contains(const Vector3 &point) const = 0;

  /**
   * @brief Get distance from point to shape boundary.
   * @param point World position.
   * @return 0 if inside, positive distance if outside.
   */
  [[nodiscard]] virtual float GetDistance(const Vector3 &point) const = 0;

  /**
   * @brief Get the shape type.
   */
  [[nodiscard]] virtual ZoneShapeType GetType() const = 0;
};

/**
 * @brief Spherical zone shape.
 */
class SphereShape : public ZoneShape {
public:
  SphereShape(const Vector3 &center, float innerRadius, float outerRadius)
      : m_Center(center), m_InnerRadius(innerRadius),
        m_OuterRadius(outerRadius) {}

  [[nodiscard]] bool Contains(const Vector3 &point) const override {
    float dist = Distance(point, m_Center);
    return dist <= m_OuterRadius;
  }

  [[nodiscard]] float GetDistance(const Vector3 &point) const override {
    float dist = Distance(point, m_Center);
    if (dist <= m_InnerRadius) {
      return 0.0f; // Full volume zone
    }
    if (dist >= m_OuterRadius) {
      return dist - m_OuterRadius;
    }
    // Normalized distance within fade zone (0-1)
    return (dist - m_InnerRadius) / (m_OuterRadius - m_InnerRadius);
  }

  [[nodiscard]] ZoneShapeType GetType() const override {
    return ZoneShapeType::Sphere;
  }

  [[nodiscard]] const Vector3 &GetCenter() const { return m_Center; }
  [[nodiscard]] float GetInnerRadius() const { return m_InnerRadius; }
  [[nodiscard]] float GetOuterRadius() const { return m_OuterRadius; }

private:
  static float Distance(const Vector3 &a, const Vector3 &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  Vector3 m_Center;
  float m_InnerRadius;
  float m_OuterRadius;
};

/**
 * @brief Axis-aligned box zone shape.
 */
class BoxShape : public ZoneShape {
public:
  /**
   * @brief Create a box zone.
   * @param min Minimum corner (x, y, z).
   * @param max Maximum corner (x, y, z).
   * @param fadeDistance Distance for volume fade at edges.
   */
  BoxShape(const Vector3 &min, const Vector3 &max, float fadeDistance = 5.0f)
      : m_Min(min), m_Max(max), m_FadeDistance(fadeDistance) {}

  [[nodiscard]] bool Contains(const Vector3 &point) const override {
    return point.x >= m_Min.x - m_FadeDistance &&
           point.x <= m_Max.x + m_FadeDistance &&
           point.y >= m_Min.y - m_FadeDistance &&
           point.y <= m_Max.y + m_FadeDistance &&
           point.z >= m_Min.z - m_FadeDistance &&
           point.z <= m_Max.z + m_FadeDistance;
  }

  [[nodiscard]] float GetDistance(const Vector3 &point) const override {
    // Distance to box surface (0 if inside core, positive outside)
    float dx =
        (std::max)(0.0f, (std::max)(m_Min.x - point.x, point.x - m_Max.x));
    float dy =
        (std::max)(0.0f, (std::max)(m_Min.y - point.y, point.y - m_Max.y));
    float dz =
        (std::max)(0.0f, (std::max)(m_Min.z - point.z, point.z - m_Max.z));

    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (dist == 0.0f) {
      return 0.0f; // Inside core box
    }
    if (dist >= m_FadeDistance) {
      return 1.0f; // Outside fade zone
    }
    return dist / m_FadeDistance; // Normalized fade
  }

  [[nodiscard]] ZoneShapeType GetType() const override {
    return ZoneShapeType::Box;
  }

  [[nodiscard]] const Vector3 &GetMin() const { return m_Min; }
  [[nodiscard]] const Vector3 &GetMax() const { return m_Max; }

private:
  Vector3 m_Min;
  Vector3 m_Max;
  float m_FadeDistance;
};

/**
 * @brief 2D polygon zone shape with height range.
 */
class PolygonShape : public ZoneShape {
public:
  /**
   * @brief Create a polygon zone.
   * @param points 2D polygon vertices (x, z plane).
   * @param minY Minimum height.
   * @param maxY Maximum height.
   * @param fadeDistance Distance for volume fade at edges.
   */
  PolygonShape(const std::vector<Vector2> &points, float minY, float maxY,
               float fadeDistance = 5.0f)
      : m_Points(points), m_MinY(minY), m_MaxY(maxY),
        m_FadeDistance(fadeDistance) {}

  [[nodiscard]] bool Contains(const Vector3 &point) const override {
    // Check height
    if (point.y < m_MinY - m_FadeDistance ||
        point.y > m_MaxY + m_FadeDistance) {
      return false;
    }

    // Check 2D polygon containment with fade distance
    float dist2D = PointToPolygonDistance(point.x, point.z);
    return dist2D <= m_FadeDistance;
  }

  [[nodiscard]] float GetDistance(const Vector3 &point) const override {
    // Height distance
    float heightDist = 0.0f;
    if (point.y < m_MinY) {
      heightDist = m_MinY - point.y;
    } else if (point.y > m_MaxY) {
      heightDist = point.y - m_MaxY;
    }

    // 2D polygon distance
    float polyDist = PointToPolygonDistance(point.x, point.z);

    // Combine distances
    float totalDist = std::sqrt(heightDist * heightDist + polyDist * polyDist);

    if (totalDist == 0.0f) {
      return 0.0f;
    }
    if (totalDist >= m_FadeDistance) {
      return 1.0f;
    }
    return totalDist / m_FadeDistance;
  }

  [[nodiscard]] ZoneShapeType GetType() const override {
    return ZoneShapeType::Polygon;
  }

private:
  // Point-in-polygon test using ray casting
  [[nodiscard]] bool PointInPolygon(float x, float z) const {
    bool inside = false;
    size_t n = m_Points.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
      if (((m_Points[i].y > z) != (m_Points[j].y > z)) &&
          (x < (m_Points[j].x - m_Points[i].x) * (z - m_Points[i].y) /
                       (m_Points[j].y - m_Points[i].y) +
                   m_Points[i].x)) {
        inside = !inside;
      }
    }
    return inside;
  }

  // Distance from point to polygon boundary
  [[nodiscard]] float PointToPolygonDistance(float x, float z) const {
    if (PointInPolygon(x, z)) {
      return 0.0f; // Inside polygon
    }

    // Find minimum distance to any edge
    float minDist = std::numeric_limits<float>::max();
    size_t n = m_Points.size();
    for (size_t i = 0; i < n; ++i) {
      size_t j = (i + 1) % n;
      float dist = PointToSegmentDistance(x, z, m_Points[i].x, m_Points[i].y,
                                          m_Points[j].x, m_Points[j].y);
      minDist = (std::min)(minDist, dist);
    }
    return minDist;
  }

  // Distance from point to line segment
  static float PointToSegmentDistance(float px, float pz, float x1, float z1,
                                      float x2, float z2) {
    float dx = x2 - x1;
    float dz = z2 - z1;
    float lengthSq = dx * dx + dz * dz;

    if (lengthSq == 0.0f) {
      // Segment is a point
      float ddx = px - x1;
      float ddz = pz - z1;
      return std::sqrt(ddx * ddx + ddz * ddz);
    }

    // Project point onto segment
    float t = (std::max)(
        0.0f, (std::min)(1.0f, ((px - x1) * dx + (pz - z1) * dz) / lengthSq));
    float projX = x1 + t * dx;
    float projZ = z1 + t * dz;

    float ddx = px - projX;
    float ddz = pz - projZ;
    return std::sqrt(ddx * ddx + ddz * ddz);
  }

  std::vector<Vector2> m_Points;
  float m_MinY;
  float m_MaxY;
  float m_FadeDistance;
};

} // namespace Orpheus
