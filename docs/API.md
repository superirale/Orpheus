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
  uint8_t priority = 128;    // 0-255, higher = never stolen by lower
  float maxDistance = 100.0f; // 3D attenuation distance
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
  "priority": 200,
  "maxDistance": 50.0,
  "parameters": { "distance": "attenuation" }
}
```

---

## Voice Pool

Virtual voices and voice stealing for CPU management.

| Method | Description |
|--------|-------------|
| `void setMaxVoices(uint32_t max)` | Set max simultaneous real voices (default: 32). |
| `void setStealBehavior(StealBehavior behavior)` | Set stealing: `Oldest`, `Furthest`, `Quietest`, `None`. |
| `uint32_t getRealVoiceCount()` | Count of currently playing voices. |
| `uint32_t getVirtualVoiceCount()` | Count of virtualized (silent) voices. |
| `uint32_t getActiveVoiceCount()` | Total real + virtual voices. |

**How it works:**
- When max voices reached, lowest-priority/least-audible voice is stolen
- High-priority sounds (255) are never stolen
- Virtual voices track playback time but produce no audio
- Voices automatically promote from virtual to real when closer/louder

**Example:**
```cpp
audio.setMaxVoices(32);
audio.setStealBehavior(StealBehavior::Quietest);

// Spawn 100 sounds - only 32 will play, rest virtualized
for (int i = 0; i < 100; i++) {
  audio.playEvent("footstep", randomPosition);
}

std::cout << "Real: " << audio.getRealVoiceCount()
          << " Virtual: " << audio.getVirtualVoiceCount();
```

---

## Mix Zones

Spatial regions that automatically apply snapshots when the listener enters.

| Method | Description |
|--------|-------------|
| `void addMixZone(name, snapshotName, pos, inner, outer, priority, fadeIn, fadeOut)` | Add a mix zone. |
| `void removeMixZone(const std::string& name)` | Remove a mix zone. |
| `void setZoneEnterCallback(fn)` | Set callback for zone entry. |
| `void setZoneExitCallback(fn)` | Set callback for zone exit. |
| `const std::string& getActiveMixZone()` | Get currently active zone name. |

**Parameters:**
- `inner`: Inner radius (full snapshot intensity)
- `outer`: Outer radius (snapshot fades out)
- `priority`: Higher priority zones override lower (0-255)

**Example:**
```cpp
// Create snapshots
audio.createSnapshot("Cave");
audio.setSnapshotBusVolume("Cave", "Music", 0.4f);

audio.createSnapshot("Combat");
audio.setSnapshotBusVolume("Combat", "Music", 0.2f);

// Create zones with priority
audio.addMixZone("cave", "Cave", {30, 0, 0}, 5.0f, 15.0f, 100);
audio.addMixZone("arena", "Combat", {60, 0, 0}, 10.0f, 25.0f, 200);

// Optional: track zone events
audio.setZoneEnterCallback([](const std::string& zone) {
  std::cout << "Entered: " << zone << "\n";
});
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

Spatial audio regions that trigger events based on listener distance. Zones use the event system, so sounds are routed through buses and affected by snapshots.

| Method | Description |
|--------|-------------|
| `void addAudioZone(const std::string& eventName, const Vector3& pos, float inner, float outer)` | Add a zone using an event. |

- **eventName**: Name of a registered event (not a file path)
- **innerRadius**: Full volume distance
- **outerRadius**: Zero volume distance (sound stops beyond this)

**Example:**
```cpp
// Register the event first
audio.registerEvent(ambientEvent);

// Create zone using event name
audio.addAudioZone("forest_ambient", {100, 0, 50}, 10.0f, 50.0f);
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
| `void setVolume(float v)` | Set immediate volume (no fade). |
| `void setTargetVolume(float v, float fadeSeconds = 0)` | Set target with configurable fade time. |
| `float getVolume() const` | Get current volume. |
| `float getTargetVolume() const` | Get target volume. |
| `void addFilter(std::shared_ptr<SoLoud::Filter> f)` | Attach a DSP filter. |

**Default buses:** `Master`, `SFX`, `Music`

---

## Snapshots

Save and restore bus mix states with smooth fade transitions.

| Method | Description |
|--------|-------------|
| `void createSnapshot(const std::string& name)` | Create a named snapshot. |
| `void setSnapshotBusVolume(const std::string& snap, const std::string& bus, float volume)` | Set a bus volume in a snapshot. |
| `void applySnapshot(const std::string& name, float fadeSeconds = 0.3f)` | Apply a snapshot with fade. |
| `void resetBusVolumes(float fadeSeconds = 0.3f)` | Reset all buses to 1.0 with fade. |
| `void resetEventVolume(const std::string& eventName, float fadeSeconds = 0.3f)` | Reset bus to event's `volumeMin` with fade. |

**Example:**
```cpp
// Create snapshots
audio.createSnapshot("Combat");
audio.setSnapshotBusVolume("Combat", "Music", 0.3f);
audio.setSnapshotBusVolume("Combat", "SFX", 1.0f);

// Enter combat - music fades down over 2 seconds
audio.applySnapshot("Combat", 2.0f);

// Exit combat - restore volumes with 1 second fade
audio.resetBusVolumes(1.0f);
// OR
audio.resetEventVolume("music", 0.5f);
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
