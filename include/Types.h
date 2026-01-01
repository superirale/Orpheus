/**
 * @file Types.h
 * @brief Core type definitions for the Orpheus Audio Engine.
 *
 * Defines fundamental types used throughout the audio engine including
 * audio handles and 3D vector types.
 */
#pragma once

#include <soloud.h>

namespace Orpheus {

/**
 * @brief Handle type for audio playback instances.
 *
 * This is an alias for SoLoud's handle type, used to reference
 * and control playing audio instances.
 */
typedef SoLoud::handle AudioHandle;

/**
 * @brief 3D vector for spatial audio positioning.
 *
 * Used for listener and sound source positions in 3D space.
 */
struct Vector3 {
  float x; ///< X coordinate
  float y; ///< Y coordinate
  float z; ///< Z coordinate
};

} // namespace Orpheus
