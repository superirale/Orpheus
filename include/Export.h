/**
 * @file Export.h
 * @brief Export/import macros for shared library builds.
 *
 * Use ORPHEUS_API on public API symbols when building as a shared library.
 */
#pragma once

// Determine export/import based on build configuration
#if defined(ORPHEUS_SHARED_BUILD)
// Building Orpheus as a shared library
#if defined(_WIN32) || defined(_WIN64)
#if defined(ORPHEUS_EXPORTS)
#define ORPHEUS_API __declspec(dllexport)
#else
#define ORPHEUS_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define ORPHEUS_API __attribute__((visibility("default")))
#else
#define ORPHEUS_API
#endif
#else
// Static library build - no export needed
#define ORPHEUS_API
#endif

// Internal linkage (not exported)
#if defined(_WIN32) || defined(_WIN64)
#define ORPHEUS_INTERNAL
#elif defined(__GNUC__) || defined(__clang__)
#define ORPHEUS_INTERNAL __attribute__((visibility("hidden")))
#else
#define ORPHEUS_INTERNAL
#endif
