# GoldenLyre 🎵

A lightweight, FMOD/Wwise-inspired audio engine built on [SoLoud](https://solhsa.com/soloud/).

## Features

- **Event System** — Register and play named audio events from code or JSON
- **3D Listeners** — Handle-based spatial audio (supports multiple listeners)
- **Audio Zones** — Event-triggered spatial audio regions with distance falloff
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

  // Play a sound
  audio.loadEventsFromFile("assets/events.json");
  audio.playEvent("explosion");

  // 3D audio
  ListenerID listener = audio.createListener();
  audio.setListenerPosition(listener, playerX, playerY, playerZ);

  // Audio zones (trigger events based on distance)
  audio.addAudioZone("ambient_forest", {100, 0, 50}, 10.0f, 50.0f);

  // Snapshots with configurable fade times
  audio.createSnapshot("Combat");
  audio.setSnapshotBusVolume("Combat", "Music", 0.3f);
  
  // Game loop
  while (running) {
    if (inCombat) audio.applySnapshot("Combat", 2.0f);  // 2 second fade
    else audio.resetBusVolumes(1.0f);                   // 1 second fade
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

## Project Structure

```
GoldenLyre/
├── include/           # Headers
│   ├── AudioManager.h # Main API
│   ├── Listener.h     # 3D listener
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
