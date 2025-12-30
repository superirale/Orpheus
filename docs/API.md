# GoldenLyre API Reference

GoldenLyre is a lightweight audio engine wrapper around SoLoud, providing FMOD/Wwise-style features.

---

## AudioManager

The main entry point for all audio operations.

### Lifecycle

| Method | Description |
|--------|-------------|
| `bool init()` | Initialize the audio engine. Returns `true` on success. |
| `void shutdown()` | Deinitialize the audio engine. |
| `void update(float dt)` | Call every frame to update 3D audio, buses, and zones. |

### Events

| Method | Description |
|--------|-------------|
| `void registerEvent(const EventDescriptor& ed)` | Register an event using a struct. |
| `bool registerEvent(const std::string& jsonString)` | Register an event from a JSON string. |
| `bool loadEventsFromFile(const std::string& jsonPath)` | Load multiple events from a JSON file. |
| `AudioHandle playEvent(const std::string& name)` | Play a registered event. Returns a handle. |

**EventDescriptor struct:**
```cpp
struct EventDescriptor {
  std::string name;
  std::string path;          // Path to .wav file
  std::string bus;           // "Master", "SFX", "Music"
  float volumeMin = 1.0f;
  float volumeMax = 1.0f;
  float pitchMin = 1.0f;
  float pitchMax = 1.0f;
  bool stream = false;       // Use WavStream for long files
};
```

**JSON format:**
```json
{
  "name": "explosion",
  "sound": "assets/sfx/explosion.wav",
  "bus": "SFX",
  "volume": [0.8, 1.0],
  "pitch": [0.9, 1.1],
  "stream": false,
  "parameters": { "distance": "attenuation" }
}
```

---

## Listeners

Handle-based 3D listener management (FMOD/Wwise style).

| Method | Description |
|--------|-------------|
| `ListenerID createListener()` | Create a new listener. Returns a handle. |
| `void destroyListener(ListenerID id)` | Remove a listener. |
| `void setListenerPosition(ListenerID id, const Vector3& pos)` | Set listener position. |
| `void setListenerPosition(ListenerID id, float x, float y, float z)` | Set listener position (floats). |
| `void setListenerVelocity(ListenerID id, const Vector3& vel)` | Set velocity for Doppler effects. |
| `void setListenerOrientation(ListenerID id, const Vector3& forward, const Vector3& up)` | Set listener orientation. |

**Example:**
```cpp
ListenerID listener = audio.createListener();
audio.setListenerPosition(listener, 0.0f, 0.0f, 0.0f);
audio.update(dt);  // Process all listeners
```

---

## Audio Zones

Spatial audio regions that play sounds based on listener distance.

| Method | Description |
|--------|-------------|
| `void addAudioZone(const std::string& soundPath, const Vector3& pos, float inner, float outer, bool stream = true)` | Add a zone. |

- **innerRadius**: Full volume distance
- **outerRadius**: Zero volume distance (sound stops beyond this)

**Example:**
```cpp
audio.addAudioZone("assets/ambient/forest.wav", {100, 0, 50}, 10.0f, 50.0f);
```

---

## Buses

Audio routing channels with volume control.

| Method | Description |
|--------|-------------|
| `void createBus(const std::string& name)` | Create a new bus. |
| `std::shared_ptr<Bus> getBus(const std::string& name)` | Get a bus by name. |

**Bus methods:**
| Method | Description |
|--------|-------------|
| `void setVolume(float v)` | Set immediate volume. |
| `void setTargetVolume(float v)` | Set target for smooth interpolation. |
| `float getVolume() const` | Get current volume. |
| `void addFilter(std::shared_ptr<SoLoud::Filter> f)` | Attach a DSP filter. |

**Default buses:** `Master`, `SFX`, `Music`

---

## Snapshots

Save and restore bus mix states with smooth transitions.

| Method | Description |
|--------|-------------|
| `void createSnapshot(const std::string& name)` | Create a named snapshot. |
| `void setSnapshotBusVolume(const std::string& snap, const std::string& bus, float volume)` | Set a bus volume in a snapshot. |
| `void applySnapshot(const std::string& name)` | Apply a snapshot (smooth transition). |

**Example:**
```cpp
audio.createSnapshot("Combat");
audio.setSnapshotBusVolume("Combat", "Music", 0.3f);
audio.setSnapshotBusVolume("Combat", "SFX", 1.0f);

audio.createSnapshot("Menu");
audio.setSnapshotBusVolume("Menu", "Music", 1.0f);
audio.setSnapshotBusVolume("Menu", "SFX", 0.5f);

// When entering combat:
audio.applySnapshot("Combat");
```

---

## Parameters

Global parameters that can drive audio behavior.

| Method | Description |
|--------|-------------|
| `void setGlobalParameter(const std::string& name, float value)` | Set a global parameter. |
| `Parameter* getParam(const std::string& name)` | Get a parameter to read or bind. |

**Parameter class:**
| Method | Description |
|--------|-------------|
| `float Get() const` | Get current value. |
| `void Set(float v)` | Set value (triggers callbacks). |
| `void Bind(std::function<void(float)> cb)` | Bind a callback for value changes. |

---

## Types

```cpp
using AudioHandle = SoLoud::handle;
using ListenerID = uint32_t;

struct Vector3 {
  float x, y, z;
};
```

---

## Quick Start

```cpp
#include "GoldenLyre.h"

int main() {
  AudioManager audio;
  audio.init();

  // Register and play event
  audio.loadEventsFromFile("assets/events.json");
  audio.playEvent("explosion");

  // Set up 3D listener
  ListenerID listener = audio.createListener();
  audio.setListenerPosition(listener, 0, 0, 0);

  // Main loop
  while (running) {
    audio.setListenerPosition(listener, playerX, playerY, playerZ);
    audio.update(deltaTime);
  }

  audio.shutdown();
  return 0;
}
```
