# Orpheus 🎵

A lightweight, FMOD/Wwise-inspired audio engine built on [SoLoud](https://solhsa.com/soloud/).

## Features

- **Event System** — Register and play named audio events from code or JSON
- **3D Listeners** — Handle-based spatial audio (supports multiple listeners)
- **Audio Zones** — Spatial audio regions with distance-based volume falloff
- **Zone-Triggered Snapshots** — Bind snapshots to zones for automatic mix changes
- **Mix Zones** — Priority-based snapshot regions with smooth transitions
- **Reverb Buses** — Environment-based spatial coloration with Freeverb DSP
- **Occlusion & Obstruction** — Physical geometry affects sound (muffling behind walls)
- **Sidechaining/Ducking** — Automatic volume reduction (music ducks during dialogue)
- **Distance Curves** — Custom rolloff (linear, logarithmic, inverse square, custom)
- **Doppler Effect** — Velocity-based pitch shift for moving 3D sounds
- **Randomization** — Random pitch/volume variation, playlist shuffle
- **Markers/Cues** — Trigger callbacks at specific audio positions
- **Interactive Music** — Stingers, transitions, beat-synced segments
- **RTPC Curves** — Parameter-to-effect automation curves
- **Profiler** — Voice count, CPU usage, memory stats
- **Zone Shapes** — Box and polygon zones (not just spheres)
- **Zone Crossfading** — Normalize volume when zones overlap
- **Bus Routing** — Organize audio into Master, SFX, Music channels
- **Snapshots** — Save/restore mix states with configurable fade times
- **Voice Pool** — Virtual voices & voice stealing (prevents CPU spikes)
- **Parameters** — Global values that can drive audio behavior
- **Error Handling** — `Result<T>` pattern for explicit error propagation
- **Logging** — Configurable log levels (Debug/Info/Warn/Error) with callbacks

## Quick Start

```cpp
#include "Orpheus.h"
using namespace Orpheus;

int main() {
  AudioManager audio;
  
  // Initialize with error handling
  auto initResult = audio.Init();
  if (initResult.IsError()) {
    std::cerr << initResult.GetError().What() << "\n";
    return -1;
  }

  // Load and play events
  audio.LoadEventsFromFile("assets/events.json");
  auto voiceResult = audio.PlayEvent("background_music");
  if (voiceResult.IsError()) {
    ORPHEUS_WARN("Failed to play: " << voiceResult.GetError().What());
  }

  // 3D listener
  ListenerID listener = audio.CreateListener();
  audio.SetListenerPosition(listener, 0, 0, 0);

  // Basic audio zone (plays sound when listener is near)
  audio.AddAudioZone("forest_ambient", {100, 0, 50}, 10.0f, 50.0f);

  // Zone-triggered snapshot
  // Automatically applies "Underwater" mix when entering the waterfall zone
  audio.CreateSnapshot("Underwater");
  audio.SetSnapshotBusVolume("Underwater", "Music", 0.3f);
  audio.AddAudioZone("waterfall", {60, 0, 0}, 5.0f, 15.0f, "Underwater");

  // Mix zones for area-based mixing
  audio.CreateSnapshot("Combat");
  audio.SetSnapshotBusVolume("Combat", "Music", 0.2f);
  audio.AddMixZone("arena", "Combat", {100, 0, 0}, 10.0f, 25.0f, 200);

  // Game loop
  while (running) {
    audio.SetListenerPosition(listener, playerX, playerY, playerZ);
    audio.Update(deltaTime);
  }

  audio.Shutdown();
}
```

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build . -j8
./orpheus_example

# Run tests
./orpheus_tests
```

**Requirements:**
- CMake 3.14+
- C++17 compiler

SoLoud, nlohmann/json, and Catch2 are fetched automatically via CMake FetchContent.

## Event JSON Format

```json
{
  "name": "footstep",
  "sound": "assets/sfx/footstep.wav",
  "bus": "SFX",
  "volume": [0.8, 1.0],
  "pitch": [0.9, 1.1]
}
```

## Audio Zones vs Mix Zones

Orpheus provides two spatial audio systems:

| Feature | Audio Zones | Mix Zones |
|---------|-------------|-----------|
| **Purpose** | Play ambient sounds | Change mix/atmosphere |
| **Triggers** | Event playback + optional snapshot | Snapshot only |
| **Priority** | N/A | Priority-based (higher wins) |
| **Best for** | Waterfalls, campfires, ambient loops | Caves, combat areas, underwater |

```cpp
// Audio Zone: plays "waterfall" sound + applies "Underwater" snapshot
audio.AddAudioZone("waterfall", {60, 0, 0}, 5.0f, 15.0f, "Underwater");

// Mix Zone: only applies "Combat" snapshot (no sound playback)
audio.AddMixZone("arena", "Combat", {100, 0, 0}, 10.0f, 25.0f, 200);
```

## Project Structure

```
Orpheus/
├── include/           # Headers
│   ├── AudioManager.h # Main API
│   ├── Error.h        # Result<T> error handling
│   ├── Log.h          # Logging system
│   ├── AudioZone.h    # Spatial audio regions
│   ├── MixZone.h      # Snapshot regions
│   ├── Bus.h          # Audio routing
│   ├── Snapshot.h     # Mix snapshots
│   └── ...
├── tests/             # Unit tests (Catch2)
├── example/           # Example usage
├── assets/            # Audio files
└── docs/              # Documentation
    └── API.md         # Full API reference
```

## Documentation

See [docs/API.md](docs/API.md) for the full API reference.

## Developer Tools

### Benchmarks

Performance testing with Google Benchmark:

```bash
cmake -B build -DORPHEUS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target orpheus_benchmarks
./build/orpheus_benchmarks
```

### Fuzzing

Fuzz JSON parsing with libFuzzer (requires Clang):

```bash
cmake -B build -DORPHEUS_BUILD_FUZZER=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build build
./build/fuzz_json_parser fuzz/corpus/ -max_total_time=60
```

### Code Coverage

Generate HTML coverage reports:

```bash
# Linux: install lcov first
sudo apt-get install lcov

cmake -B build -DORPHEUS_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/orpheus_tests
cmake --build build --target coverage
open build/coverage/html/index.html
```

## License

MIT


