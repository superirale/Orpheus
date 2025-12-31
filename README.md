# GoldenLyre 🎵

A lightweight, FMOD/Wwise-inspired audio engine built on [SoLoud](https://solhsa.com/soloud/).

## Features

- **Event System** — Register and play named audio events from code or JSON
- **3D Listeners** — Handle-based spatial audio (supports multiple listeners)
- **Audio Zones** — Spatial audio regions with distance-based volume falloff
- **Zone-Triggered Snapshots** — Bind snapshots to zones for automatic mix changes
- **Mix Zones** — Priority-based snapshot regions with smooth transitions
- **Reverb Buses** — Environment-based spatial coloration with Freeverb DSP
- **Bus Routing** — Organize audio into Master, SFX, Music channels
- **Snapshots** — Save/restore mix states with configurable fade times
- **Voice Pool** — Virtual voices & voice stealing (prevents CPU spikes)
- **Parameters** — Global values that can drive audio behavior

## Quick Start

```cpp
#include "GoldenLyre.h"

int main() {
  AudioManager audio;
  audio.init();

  // Load and play events
  audio.loadEventsFromFile("assets/events.json");
  audio.playEvent("background_music");

  // 3D listener
  ListenerID listener = audio.createListener();
  audio.setListenerPosition(listener, 0, 0, 0);

  // Basic audio zone (plays sound when listener is near)
  audio.addAudioZone("forest_ambient", {100, 0, 50}, 10.0f, 50.0f);

  // Zone-triggered snapshot (NEW!)
  // Automatically applies "Underwater" mix when entering the waterfall zone
  audio.createSnapshot("Underwater");
  audio.setSnapshotBusVolume("Underwater", "Music", 0.3f);
  audio.addAudioZone("waterfall", {60, 0, 0}, 5.0f, 15.0f, "Underwater");

  // Mix zones for area-based mixing
  audio.createSnapshot("Combat");
  audio.setSnapshotBusVolume("Combat", "Music", 0.2f);
  audio.addMixZone("arena", "Combat", {100, 0, 0}, 10.0f, 25.0f, 200);

  // Game loop
  while (running) {
    audio.setListenerPosition(listener, playerX, playerY, playerZ);
    audio.update(deltaTime);
  }

  audio.shutdown();
}
```

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build . -j8
./goldenlyre_example
```

**Requirements:**
- CMake 3.14+
- C++17 compiler

SoLoud and nlohmann/json are fetched automatically via CMake FetchContent.

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

GoldenLyre provides two spatial audio systems:

| Feature | Audio Zones | Mix Zones |
|---------|-------------|-----------|
| **Purpose** | Play ambient sounds | Change mix/atmosphere |
| **Triggers** | Event playback + optional snapshot | Snapshot only |
| **Priority** | N/A | Priority-based (higher wins) |
| **Best for** | Waterfalls, campfires, ambient loops | Caves, combat areas, underwater |

```cpp
// Audio Zone: plays "waterfall" sound + applies "Underwater" snapshot
audio.addAudioZone("waterfall", {60, 0, 0}, 5.0f, 15.0f, "Underwater");

// Mix Zone: only applies "Combat" snapshot (no sound playback)
audio.addMixZone("arena", "Combat", {100, 0, 0}, 10.0f, 25.0f, 200);
```

## Project Structure

```
GoldenLyre/
├── include/           # Headers
│   ├── AudioManager.h # Main API
│   ├── AudioZone.h    # Spatial audio regions
│   ├── MixZone.h      # Snapshot regions
│   ├── Bus.h          # Audio routing
│   ├── Snapshot.h     # Mix snapshots
│   └── ...
├── example/           # Example usage
├── assets/            # Audio files
└── docs/              # Documentation
    └── API.md         # Full API reference
```

## Documentation

See [docs/API.md](docs/API.md) for the full API reference.

## License

MIT
