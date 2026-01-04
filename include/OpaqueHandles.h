/**
 * @file OpaqueHandles.h
 * @brief Opaque handle types for native audio engine objects.
 *
 * These types wrap internal pointers for advanced users who need
 * direct access to the underlying audio backend (SoLoud).
 *
 * @warning Casting these handles requires including SoLoud headers
 *          and understanding the audio backend implementation.
 */
#pragma once

namespace Orpheus {

/**
 * @brief Opaque handle to a native audio bus.
 *
 * For advanced users, cast to access the underlying implementation:
 * @code
 * #include <soloud_bus.h>
 * SoLoud::Bus* bus = static_cast<SoLoud::Bus*>(handle.ptr);
 * @endcode
 */
struct NativeBusHandle {
  void *ptr = nullptr;

  explicit operator bool() const { return ptr != nullptr; }
};

/**
 * @brief Opaque handle to a native audio filter.
 *
 * For advanced users, cast to access the underlying filter:
 * @code
 * #include <soloud_biquadresonantfilter.h>
 * auto* filter = static_cast<SoLoud::BiquadResonantFilter*>(handle.ptr);
 * @endcode
 */
struct NativeFilterHandle {
  void *ptr = nullptr;

  explicit operator bool() const { return ptr != nullptr; }
};

/**
 * @brief Opaque handle to the native audio engine.
 *
 * For advanced users, cast to access the underlying engine:
 * @code
 * #include <soloud.h>
 * SoLoud::Soloud* engine = static_cast<SoLoud::Soloud*>(handle.ptr);
 * @endcode
 */
struct NativeEngineHandle {
  void *ptr = nullptr;

  explicit operator bool() const { return ptr != nullptr; }
};

} // namespace Orpheus
