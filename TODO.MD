# TODO

## Completed ✅
- [x] Reverb / DSP Filters - Add SoLoud filters to simulate room reverb per zone
- [x] Occlusion/Obstruction - Apply lowpass filters when obstacles block line of sight
- [x] Per-sound DSP chains - BiquadResonantFilter attached to audio sources

## High Priority 🔥
- [x] Sidechaining/Ducking - Music ducks when dialogue plays
- [x] Distance Curves - Custom rolloff (linear, logarithmic, custom)
- [x] Doppler Effect - Velocity-based pitch shift for moving 3D sounds
- [x] Randomization - Random pitch/volume variation, playlist shuffle
- [x] Markers/Cues - Trigger callbacks at specific audio positions

## Medium Priority
- [x] Interactive Music - Stingers, transitions, beat-synced segments
- [x] RTPC Curves - Parameter-to-effect automation curves
- [x] Profiler - Voice count, CPU usage, memory stats
- [x] Zone Shapes - Replace simple spheres with boxes or polygons
- [ ] Crossfading - Normalize volume when multiple zones overlap
- [ ] Dynamic Zones - Move or scale zones during gameplay
- [ ] Compressor/Limiter - Dynamic range control on buses

## Low Priority
- [ ] Convolution Reverb - Impulse response based (realistic spaces)
- [ ] HDR Audio - Loudness normalization
- [ ] Surround/LFE - 5.1/7.1 speaker layouts
- [ ] Ray-traced Acoustics - Advanced propagation simulation
- [ ] Audio Codec - Vorbis/Opus compression support




# Orpheus Improvements Roadmap

## High Priority

| Area | Current State | Improvement |
|------|---------------|-------------|
| ~~**Namespace**~~ | ~~Global namespace~~ | ~~Wrap everything in `namespace Orpheus { }`~~ ✅ |
| ~~**Error Handling**~~ | ~~`std::cerr` + return `false`~~ | ~~Proper error codes/`Result<T>` type or exceptions~~ ✅ |
| ~~**Logging**~~ | ~~None~~ | ~~Add logging levels (Debug/Info/Warn/Error) with callbacks~~ ✅ |
| ~~**Unit Tests**~~ | ~~None~~ | ~~Add GoogleTest or Catch2 with CI integration~~ ✅ |
| ~~**Documentation**~~ | ~~Basic API.md~~ | ~~Add Doxygen comments to headers~~ ✅ |

## Medium Priority

| Area | Improvement |
|------|-------------|
| ~~**Thread Safety**~~ | ~~Audit and document thread-safety guarantees per method~~ ✅ |
| ~~**PIMPL Pattern**~~ | ~~Hide implementation details for ABI stability~~ ✅ |
| ~~**Const Correctness**~~ | ~~Add `[[nodiscard]]` to getters, review `const` usage~~ ✅ |
| ~~**Smart Pointers**~~ | ~~Replace raw pointers with `std::unique_ptr`/`std::shared_ptr` consistently~~ ✅ |
| ~~**Static Analysis**~~ | ~~Integrate clang-tidy, cppcheck into CI~~ ✅ |

## Nice to Have

- ~~**Benchmarks** - Performance tests for voice management, DSP~~ ✅
- ~~**Fuzzing** - Fuzz JSON parsing with AFL/libFuzzer~~ ✅
- ~~**Code Coverage** - Track with gcov/llvm-cov~~ ✅
- ~~**GitHub Actions** - CI/CD pipeline for builds + tests~~ ✅
- ~~**Version Macros** - `ORPHEUS_VERSION_MAJOR`, etc.~~ ✅
- ~~**Export Macros** - `ORPHEUS_API` for shared library builds~~ ✅
