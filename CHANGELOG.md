# Changelog

All notable changes to Orpheus will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.6] - 2026-01-13

### Added
- **Core Audio**: Added `startDelay` to events for delayed playback.
- **Playlists**: Added `interval` (delay between tracks) and `loopPlaylist` (repeat behavior).
- **Playlists**: Complete support for `Sequential`, `Random`, and `Shuffle` modes with intervals.
- **Example**: `example_2.cpp` demonstrating delays and looping.

## [0.0.1] - 2026-01-02

### Added

#### Core Audio
- Event system with JSON-based registration and playback
- Bus routing (Master, SFX, Music channels)
- Snapshot system for mix state save/restore with configurable fade times
- Voice pool with virtual voices and voice stealing (prevents CPU spikes)
- Global parameters that drive audio behavior
- Vorbis/Opus audio codec support

#### 3D Audio
- Handle-based listener system (supports multiple listeners)
- Audio zones with distance-based volume falloff (sphere, box, polygon shapes)
- Zone crossfading for smooth transitions when zones overlap
- Dynamic zones (move or scale during gameplay)
- Mix zones for priority-based snapshot regions
- Doppler effect for moving 3D sounds
- Distance curves (linear, logarithmic, inverse square, custom)

#### DSP & Effects
- Occlusion & obstruction (physical geometry affects sound)
- Sidechaining/ducking (music ducks during dialogue)
- Compressor/limiter on buses
- Reverb buses with Freeverb DSP
- Convolution reverb with impulse response support

#### Advanced Features
- Interactive music (stingers, transitions, beat-synced segments)
- RTPC curves for parameter-to-effect automation
- Markers/cues with callbacks at specific audio positions
- Randomization (pitch/volume variation, playlist shuffle)
- HDR audio with LUFS loudness normalization (ITU-R BS.1770)
- Surround audio (5.1/7.1) with VBAP panning
- Ray-traced acoustics with reflections and material absorption

#### Developer Experience
- `Result<T>` error handling pattern
- Configurable logging (Debug/Info/Warn/Error) with callbacks
- Profiler (voice count, CPU usage, memory stats)
- Unit tests with Catch2
- Benchmarks with Google Benchmark
- JSON fuzzing with libFuzzer
- Code coverage with lcov/llvm-cov
- GitHub Actions CI/CD pipeline
- Doxygen documentation in headers

#### Build & Platform
- CMake build system with FetchContent for dependencies
- C++17 required
- Cross-platform (macOS, Linux, Windows)
- Windows build compatibility fixes for min/max macros
