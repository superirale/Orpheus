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
| `void addAudioZone(eventName, pos, inner, outer)` | Add a zone using an event. |
| `void addAudioZone(eventName, pos, inner, outer, snapshotName, fadeIn, fadeOut)` | Add a zone with snapshot binding. |

- **eventName**: Name of a registered event (not a file path)
- **innerRadius**: Full volume distance
- **outerRadius**: Zero volume distance (sound stops beyond this)
- **snapshotName**: Snapshot to apply when listener enters zone (optional)
- **fadeIn**: Fade time (seconds) when entering zone (default: 0.5)
- **fadeOut**: Fade time (seconds) when exiting zone (default: 0.5)

**Basic Example:**
```cpp
// Register the event first
audio.registerEvent(ambientEvent);

// Create zone using event name
audio.addAudioZone("forest_ambient", {100, 0, 50}, 10.0f, 50.0f);
```

### Zone-Triggered Snapshots

You can bind snapshots directly to AudioZones. This applies the snapshot when the listener enters the zone and reverts it when they exit.

```cpp
// Create an underwater snapshot
audio.createSnapshot("Underwater");
audio.setSnapshotBusVolume("Underwater", "Music", 0.4f);
audio.setSnapshotBusVolume("Underwater", "SFX", 0.6f);

// Register the waterfall ambient event
audio.registerEvent(waterfallEvent);

// Zone that triggers "Underwater" snapshot when listener is near
audio.addAudioZone("waterfall", {100, 0, 0}, 10.0f, 20.0f, "Underwater");

// With custom fade times (1s fade in, 2s fade out)
audio.addAudioZone("cave_drip", {50, 0, 0}, 5.0f, 15.0f, "Cave", 1.0f, 2.0f);
```

**How it works:**
- When the listener enters the zone (distance < outerRadius), the bound snapshot is applied
- When the listener exits the zone (distance > outerRadius), bus volumes reset to default
- The event audio plays with volume attenuation based on distance (independent of snapshot)

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

## Reverb Buses

Reverb buses provide environment-based spatial coloration without duplicating effects per sound instance. Sounds send signal **to** reverb buses (not play on them), enabling efficient shared DSP.

### Core API

| Method | Description |
|--------|-------------|
| `bool createReverbBus(name, roomSize, damp, wet, width)` | Create a reverb bus with custom parameters. |
| `bool createReverbBus(name, ReverbPreset)` | Create a reverb bus from a preset. |
| `shared_ptr<ReverbBus> getReverbBus(name)` | Get a reverb bus by name. |
| `void setReverbParams(name, wet, roomSize, damp, fadeTime)` | Adjust reverb parameters with fade. |
| `void addReverbZone(name, reverbBusName, pos, inner, outer, priority)` | Add a spatial reverb influence zone. |
| `void removeReverbZone(name)` | Remove a reverb zone. |
| `void setSnapshotReverbParams(snapshot, reverbBus, wet, roomSize, damp, width)` | Control reverb via snapshots. |

### Reverb Presets

| Preset | Description |
|--------|-------------|
| `ReverbPreset::Room` | Small room - short decay, high damping |
| `ReverbPreset::Hall` | Concert hall - medium decay |
| `ReverbPreset::Cave` | Large cave - long decay, low damping |
| `ReverbPreset::Cathedral` | Cathedral - very long decay, wide stereo |
| `ReverbPreset::Underwater` | Underwater - heavy wet, high damping |

### Reverb Zones

Reverb zones define spatial regions where reverb influence fades in/out based on listener distance.

- **innerRadius**: Full reverb influence at this distance
- **outerRadius**: Zero influence beyond this distance
- **priority**: Higher priority zones take precedence in overlaps

**Example:**
```cpp
// Create reverb buses
audio.createReverbBus("CaveReverb", ReverbPreset::Cave);
audio.createReverbBus("HallReverb", ReverbPreset::Hall);

// Create reverb zones
audio.addReverbZone("cave_entrance", "CaveReverb", {50, 0, 0}, 10.0f, 30.0f, 150);
audio.addReverbZone("arena_hall", "HallReverb", {100, 0, 0}, 15.0f, 40.0f, 100);

// Manual parameter control with fade
audio.setReverbParams("CaveReverb", 0.8f, 0.9f, 0.3f, 2.0f);  // 2 second fade

// Snapshot-based reverb control
audio.createSnapshot("Underwater");
audio.setSnapshotReverbParams("Underwater", "CaveReverb", 0.95f, 0.7f, 0.8f, 0.5f);
audio.applySnapshot("Underwater", 1.0f);

// Game loop - reverb zones update automatically
while (running) {
  audio.setListenerPosition(listener, playerX, playerY, playerZ);
  audio.update(dt);  // Reverb wet level fades based on zone proximity
}
```

### DSP Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| `wet` | 0.0-1.0 | Wet/dry mix (0 = dry, 1 = fully wet) |
| `roomSize` | 0.0-1.0 | Room size (larger = longer decay) |
| `damp` | 0.0-1.0 | High frequency damping (higher = darker sound) |
| `width` | 0.0-1.0 | Stereo width (0 = mono, 1 = full stereo) |
| `freeze` | bool | Infinite sustain mode |

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
