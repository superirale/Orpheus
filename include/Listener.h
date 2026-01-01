/**
 * @file Listener.h
 * @brief Audio listener definition for 3D spatial audio.
 *
 * Defines the Listener struct representing the player/camera position
 * and orientation for spatial audio calculations.
 */
#pragma once

#include <cstdint>

namespace Orpheus {

/**
 * @brief Unique identifier for audio listeners.
 */
using ListenerID = uint32_t;

/**
 * @brief Represents an audio listener in 3D space.
 *
 * The listener is typically the player or camera position.
 * Its position, velocity, and orientation are used to calculate
 * 3D panning, distance attenuation, and Doppler effects.
 */
struct Listener {
  ListenerID id = 0;                  ///< Unique identifier
  float posX = 0, posY = 0, posZ = 0; ///< Position in world space
  float velX = 0, velY = 0, velZ = 0; ///< Velocity for Doppler calculations
  float forwardX = 0, forwardY = 0, forwardZ = -1; ///< Forward direction vector
  float upX = 0, upY = 1, upZ = 0;                 ///< Up direction vector
  bool active = true; ///< Whether listener is active
};

} // namespace Orpheus
