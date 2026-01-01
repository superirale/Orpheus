# Orpheus API Reference

Orpheus is a lightweight audio engine wrapper around SoLoud, providing FMOD/Wwise-style features.

---

## AudioManager

The main entry point for all audio operations.

### Lifecycle

| Method | Description |
|--------|-------------|
| `Status Init()` | Initialize the audio engine. Returns `Ok()` on success, `Error` on failure. |
| `void Shutdown()` | Deinitialize the audio engine. |
| `void Update(float dt)` | Call every frame to update 3D audio, buses, and zones. |

### Events

| Method | Description |
|--------|-------------|
| `void RegisterEvent(const EventDescriptor& ed)` | Register an event using a struct. |
| `Status RegisterEvent(const std::string& jsonString)` | Register an event from a JSON string. |
| `Status LoadEventsFromFile(const std::string& jsonPath)` | Load multiple events from a JSON file. |
| `Result<VoiceID> PlayEvent(const std::string& name)` | Play a registered event. Returns a voice ID on success. |

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
| `void SetMaxVoices(uint32_t max)` | Set max simultaneous real voices (default: 32). |
| `void SetStealBehavior(StealBehavior behavior)` | Set stealing: `Oldest`, `Furthest`, `Quietest`, `None`. |
| `uint32_t GetRealVoiceCount()` | Count of currently playing voices. |
| `uint32_t GetVirtualVoiceCount()` | Count of virtualized (silent) voices. |
| `uint32_t GetActiveVoiceCount()` | Total real + virtual voices. |

**How it works:**
- When max voices reached, lowest-priority/least-audible voice is stolen
- High-priority sounds (255) are never stolen
- Virtual voices track playback time but produce no audio
- Voices automatically promote from virtual to real when closer/louder

**Example:**
```cpp
audio.SetMaxVoices(32);
audio.SetStealBehavior(StealBehavior::Quietest);

// Spawn 100 sounds - only 32 will play, rest virtualized
for (int i = 0; i < 100; i++) {
  audio.PlayEvent("footstep", randomPosition);
}

std::cout << "Real: " << audio.GetRealVoiceCount()
          << " Virtual: " << audio.GetVirtualVoiceCount();
```

---

## Mix Zones

Spatial regions that automatically apply snapshots when the listener enters.

| Method | Description |
|--------|--------------|
| `void AddMixZone(name, snapshotName, pos, inner, outer, priority, fadeIn, fadeOut)` | Add a mix zone. |
| `void RemoveMixZone(const std::string& name)` | Remove a mix zone. |
| `void SetZoneEnterCallback(fn)` | Set callback for zone entry. |
| `void SetZoneExitCallback(fn)` | Set callback for zone exit. |
| `const std::string& GetActiveMixZone()` | Get currently active zone name. |

**Parameters:**
- `inner`: Inner radius (full snapshot intensity)
- `outer`: Outer radius (snapshot fades out)
- `priority`: Higher priority zones override lower (0-255)

**Example:**
```cpp
// Create snapshots
audio.CreateSnapshot("Cave");
audio.SetSnapshotBusVolume("Cave", "Music", 0.4f);

audio.CreateSnapshot("Combat");
audio.SetSnapshotBusVolume("Combat", "Music", 0.2f);

// Create zones with priority
audio.AddMixZone("cave", "Cave", {30, 0, 0}, 5.0f, 15.0f, 100);
audio.AddMixZone("arena", "Combat", {60, 0, 0}, 10.0f, 25.0f, 200);

// Optional: track zone events
audio.SetZoneEnterCallback([](const std::string& zone) {
  std::cout << "Entered: " << zone << "\n";
});
```

---

## Listeners

Handle-based 3D listener management (FMOD/Wwise style).

| Method | Description |
|--------|--------------|
| `ListenerID CreateListener()` | Create a new listener. Returns a handle. |
| `void DestroyListener(ListenerID id)` | Remove a listener. |
| `void SetListenerPosition(ListenerID id, const Vector3& pos)` | Set listener position. |
| `void SetListenerPosition(ListenerID id, float x, float y, float z)` | Set listener position (floats). |
| `void SetListenerVelocity(ListenerID id, const Vector3& vel)` | Set velocity for Doppler effects. |
| `void SetListenerOrientation(ListenerID id, const Vector3& forward, const Vector3& up)` | Set listener orientation. |

**Example:**
```cpp
ListenerID listener = audio.CreateListener();
audio.SetListenerPosition(listener, 0.0f, 0.0f, 0.0f);
audio.Update(dt);  // Process all listeners
```

---

## Audio Zones

Spatial audio regions that trigger events based on listener distance. Zones use the event system, so sounds are routed through buses and affected by snapshots.

| Method | Description |
|--------|-------------|
| `void AddAudioZone(eventName, pos, inner, outer)` | Add a sphere zone using an event. |
| `void AddAudioZone(eventName, pos, inner, outer, snapshotName, fadeIn, fadeOut)` | Add a sphere zone with snapshot binding. |
| `void AddBoxZone(eventName, min, max, fadeDistance)` | Add an axis-aligned box zone. |
| `void AddPolygonZone(eventName, points, minY, maxY, fadeDistance)` | Add a 2D polygon zone with height range. |

- **eventName**: Name of a registered event (not a file path)
- **innerRadius**: Full volume distance
- **outerRadius**: Zero volume distance (sound stops beyond this)
- **snapshotName**: Snapshot to apply when listener enters zone (optional)
- **fadeIn**: Fade time (seconds) when entering zone (default: 0.5)
- **fadeOut**: Fade time (seconds) when exiting zone (default: 0.5)

### Zone Crossfading

When multiple zones overlap, volumes can be normalized to prevent clipping:

| Method | Description |
|--------|-------------|
| `void SetZoneCrossfadeEnabled(bool enabled)` | Enable/disable volume normalization |
| `bool IsZoneCrossfadeEnabled() const` | Check if crossfading is enabled |

Crossfading is **enabled by default**. When enabled:
- Zone volumes are summed
- If total > 1.0, each volume is scaled proportionally
- Example: Zone A = 0.8, Zone B = 0.6 → normalized to A = 0.57, B = 0.43

### Dynamic Zones

Zones can be moved or scaled during gameplay:

| Method | Description |
|--------|-------------|
| `AudioZone* GetZone(const std::string& eventName)` | Get a zone by event name |
| `void SetZonePosition(eventName, pos)` | Move a zone to a new position |
| `void SetZoneRadii(eventName, inner, outer)` | Change zone radii |

**Example:**
```cpp
// Move a zone (e.g., following a moving sound source)
audio.SetZonePosition("waterfall", {newX, newY, newZ});

// Scale a zone (e.g., growing/shrinking ambient area)
audio.SetZoneRadii("campfire", 5.0f, 20.0f);

// Or access directly
if (AudioZone* zone = audio.GetZone("ambient")) {
  zone->SetPosition(pos);
  zone->SetRadii(10.0f, 40.0f);
}
```

**Basic Example:**
```cpp
// Register the event first
audio.RegisterEvent(ambientEvent);

// Create zone using event name
audio.AddAudioZone("forest_ambient", {100, 0, 50}, 10.0f, 50.0f);
```

### Zone-Triggered Snapshots

You can bind snapshots directly to AudioZones. This applies the snapshot when the listener enters the zone and reverts it when they exit.

```cpp
// Create an underwater snapshot
audio.CreateSnapshot("Underwater");
audio.SetSnapshotBusVolume("Underwater", "Music", 0.4f);
audio.SetSnapshotBusVolume("Underwater", "SFX", 0.6f);

// Register the waterfall ambient event
audio.RegisterEvent(waterfallEvent);

// Zone that triggers "Underwater" snapshot when listener is near
audio.AddAudioZone("waterfall", {100, 0, 0}, 10.0f, 20.0f, "Underwater");

// With custom fade times (1s fade in, 2s fade out)
audio.AddAudioZone("cave_drip", {50, 0, 0}, 5.0f, 15.0f, "Cave", 1.0f, 2.0f);
```

**How it works:**
- When the listener enters the zone (distance < outerRadius), the bound snapshot is applied
- When the listener exits the zone (distance > outerRadius), bus volumes reset to default
- The event audio plays with volume attenuation based on distance (independent of snapshot)

---

## Buses

Audio routing channels with volume control.

| Method | Description |
|--------|--------------|
| `void CreateBus(const std::string& name)` | Create a new bus. |
| `Result<std::shared_ptr<Bus>> GetBus(const std::string& name)` | Get a bus by name. Returns `Error` if not found. |

**Bus methods:**
| Method | Description |
|--------|--------------|
| `void SetVolume(float v)` | Set immediate volume (no fade). |
| `void SetTargetVolume(float v, float fadeSeconds = 0)` | Set target with configurable fade time. |
| `float GetVolume() const` | Get current volume. |
| `float GetTargetVolume() const` | Get target volume. |
| `void AddFilter(std::shared_ptr<SoLoud::Filter> f)` | Attach a DSP filter. |

**Default buses:** `Master`, `SFX`, `Music`

---

## Snapshots

Save and restore bus mix states with smooth fade transitions.

| Method | Description |
|--------|--------------|
| `void CreateSnapshot(const std::string& name)` | Create a named snapshot. |
| `void SetSnapshotBusVolume(const std::string& snap, const std::string& bus, float volume)` | Set a bus volume in a snapshot. |
| `Status ApplySnapshot(const std::string& name, float fadeSeconds = 0.3f)` | Apply a snapshot with fade. Returns `Error` if not found. |
| `void ResetBusVolumes(float fadeSeconds = 0.3f)` | Reset all buses to 1.0 with fade. |
| `void ResetEventVolume(const std::string& eventName, float fadeSeconds = 0.3f)` | Reset bus to event's `volumeMin` with fade. |

**Example:**
```cpp
// Create snapshots
audio.CreateSnapshot("Combat");
audio.SetSnapshotBusVolume("Combat", "Music", 0.3f);
audio.SetSnapshotBusVolume("Combat", "SFX", 1.0f);

// Enter combat - music fades down over 2 seconds
audio.ApplySnapshot("Combat", 2.0f);

// Exit combat - restore volumes with 1 second fade
audio.ResetBusVolumes(1.0f);
// OR
audio.ResetEventVolume("music", 0.5f);
```

---

## Reverb Buses

Reverb buses provide environment-based spatial coloration without duplicating effects per sound instance. Sounds send signal **to** reverb buses (not play on them), enabling efficient shared DSP.

### Core API

| Method | Description |
|--------|--------------|
| `Status CreateReverbBus(name, roomSize, damp, wet, width)` | Create a reverb bus with custom parameters. |
| `Status CreateReverbBus(name, ReverbPreset)` | Create a reverb bus from a preset. |
| `Result<shared_ptr<ReverbBus>> GetReverbBus(name)` | Get a reverb bus by name. Returns `Error` if not found. |
| `void SetReverbParams(name, wet, roomSize, damp, fadeTime)` | Adjust reverb parameters with fade. |
| `void AddReverbZone(name, reverbBusName, pos, inner, outer, priority)` | Add a spatial reverb influence zone. |
| `void RemoveReverbZone(name)` | Remove a reverb zone. |
| `void SetSnapshotReverbParams(snapshot, reverbBus, wet, roomSize, damp, width)` | Control reverb via snapshots. |

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
audio.CreateReverbBus("CaveReverb", ReverbPreset::Cave);
audio.CreateReverbBus("HallReverb", ReverbPreset::Hall);

// Create reverb zones
audio.AddReverbZone("cave_entrance", "CaveReverb", {50, 0, 0}, 10.0f, 30.0f, 150);
audio.AddReverbZone("arena_hall", "HallReverb", {100, 0, 0}, 15.0f, 40.0f, 100);

// Manual parameter control with fade
audio.SetReverbParams("CaveReverb", 0.8f, 0.9f, 0.3f, 2.0f);  // 2 second fade

// Snapshot-based reverb control
audio.CreateSnapshot("Underwater");
audio.SetSnapshotReverbParams("Underwater", "CaveReverb", 0.95f, 0.7f, 0.8f, 0.5f);
audio.ApplySnapshot("Underwater", 1.0f);

// Game loop - reverb zones update automatically
while (running) {
  audio.SetListenerPosition(listener, playerX, playerY, playerZ);
  audio.Update(dt);  // Reverb wet level fades based on zone proximity
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

## Occlusion & Obstruction

Simulate how physical geometry affects sound propagation between sources and listeners.

### Core Concepts

- **Obstruction**: Partial blocking (thin walls, doors) → reduced high frequencies
- **Occlusion**: Full blocking (thick walls, terrain) → strong low-pass + volume reduction

### Core API

| Method | Description |
|--------|-------------|
| `void SetOcclusionQueryCallback(callback)` | Set callback for game-provided raycasts |
| `void RegisterOcclusionMaterial(mat)` | Register a custom material |
| `void SetOcclusionEnabled(bool)` | Enable/disable occlusion processing |
| `void SetOcclusionThreshold(float)` | Set obstruction→occlusion threshold (0-1) |
| `void SetOcclusionSmoothingTime(float)` | Set transition smoothing (seconds) |
| `void SetOcclusionUpdateRate(float hz)` | Set query rate (Hz) |
| `void SetOcclusionLowPassRange(min, max)` | Set filter frequency range |
| `void SetOcclusionVolumeReduction(float)` | Set max volume reduction (0-1) |

### Built-in Materials

| Material | Obstruction | Description |
|----------|-------------|-------------|
| Glass | 0.2 | Very light blocking |
| Wood | 0.3 | Light blocking |
| Metal | 0.5 | Medium blocking |
| Concrete | 0.8 | Heavy blocking |
| Terrain | 1.0 | Full blocking |

### Example Usage

```cpp
// Set up occlusion query callback (game provides raycasts)
audio.SetOcclusionQueryCallback(
  [](const Vector3& source, const Vector3& listener) -> std::vector<OcclusionHit> {
    std::vector<OcclusionHit> hits;
    
    // Perform raycast in your physics engine
    for (auto& hit : physics.raycast(source, listener)) {
      hits.push_back({hit.materialName, hit.thickness});
    }
    return hits;
  });

// Configure occlusion
audio.SetOcclusionEnabled(true);
audio.SetOcclusionThreshold(0.7f);      // 70% obstruction → occlusion kicks in
audio.SetOcclusionSmoothingTime(0.1f);  // 100ms transitions
audio.SetOcclusionLowPassRange(400.0f, 22000.0f);  // Filter range
audio.SetOcclusionVolumeReduction(0.5f);  // Max 50% volume reduction

// Register custom materials
audio.RegisterOcclusionMaterial({"BrickWall", 0.7f, 0.2f});
```

### DSP Mapping

| Factor | Volume | LowPass Freq |
|--------|--------|--------------|
| Unoccluded (0.0) | 100% | 22 kHz |
| Obstructed (0.5) | 75% | ~4 kHz |
| Occluded (1.0) | 50% | 400 Hz |

---

## Sidechaining / Ducking

Automatically reduce the volume of one bus when audio is playing on another (e.g., music ducks during dialogue).

### Core API

| Method | Description |
|--------|-------------|
| `void AddDuckingRule(target, sidechain, duckLevel, attack, release, hold)` | Add a ducking rule |
| `void RemoveDuckingRule(target, sidechain)` | Remove a ducking rule |
| `bool IsDucking(targetBus)` | Check if a bus is currently being ducked |

### DuckingRule Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `targetBus` | - | Bus to duck (e.g., "Music") |
| `sidechainBus` | - | Bus that triggers ducking (e.g., "Dialogue") |
| `duckLevel` | 0.3 | Target volume when ducked (0.0-1.0) |
| `attackTime` | 0.1s | Fade down time |
| `releaseTime` | 0.5s | Fade up time |
| `holdTime` | 0.1s | Hold ducked level after sidechain stops |

### Example

```cpp
// Duck music when dialogue plays
audio.AddDuckingRule("Music", "Dialogue", 0.3f, 0.1f, 0.5f, 0.1f);

// Check if music is being ducked
if (audio.IsDucking("Music")) {
  // Show "dialogue playing" indicator
}

// Game loop - ducking updates automatically
while (running) {
  audio.Update(dt);
}
```

---

## Distance Curves

Custom rolloff curves for 3D audio attenuation.

### DistanceCurve Enum

| Curve | Formula | Description |
|-------|---------|-------------|
| `Linear` | `1 - dist/max` | Simple linear falloff |
| `Logarithmic` | `1 - log10(1+9*dist)` | Natural hearing perception |
| `InverseSquare` | `1 / (1 + dist²)` | Physics-based falloff |
| `Exponential` | `e^(-dist*3)` | Steep exponential falloff |
| `Custom` | User function | Full control via callback |

### DistanceSettings Struct

| Parameter | Default | Description |
|-----------|---------|-------------|
| `curve` | Linear | Curve type from enum |
| `minDistance` | 1.0 | Distance where attenuation starts |
| `maxDistance` | 100.0 | Distance where sound is silent |
| `rolloffFactor` | 1.0 | Curve steepness multiplier |
| `customCurve` | - | Custom function (0-1 input, 0-1 output) |

### Example

```cpp
// Per-voice configuration
Voice voice;
voice.distanceSettings.curve = DistanceCurve::Logarithmic;
voice.distanceSettings.minDistance = 2.0f;   // Full volume at <=2m
voice.distanceSettings.maxDistance = 50.0f;  // Silent at >=50m
voice.distanceSettings.rolloffFactor = 1.5f; // Steeper curve

// Custom curve function
voice.distanceSettings.curve = DistanceCurve::Custom;
voice.distanceSettings.customCurve = [](float dist) {
  return 1.0f - std::pow(dist, 0.5f); // Square root falloff
};
```

---

## Doppler Effect

Velocity-based pitch shift for moving 3D sounds.

### Core API

| Method | Description |
|--------|-------------|
| `void SetVoiceVelocity(VoiceID id, const Vector3& vel)` | Set voice velocity for Doppler calculation |
| `void SetDopplerEnabled(bool enabled)` | Enable/disable Doppler processing globally |
| `void SetSpeedOfSound(float speed)` | Set speed of sound in world units/sec (default: 343) |
| `void SetDopplerFactor(float factor)` | Exaggeration factor (1.0 = realistic, >1 = exaggerated) |

### How It Works

- Sounds approaching the listener have higher pitch
- Sounds receding from the listener have lower pitch
- Formula: `pitch = speedOfSound / (speedOfSound + relativeVelocity)`
- Pitch clamped to 0.5x – 2.0x range

### Example

```cpp
// Enable Doppler (on by default)
audio.SetDopplerEnabled(true);
audio.SetSpeedOfSound(343.0f);  // Speed of sound in m/s
audio.SetDopplerFactor(1.0f);   // Realistic

// Play a moving sound
VoiceID car = audio.PlayEvent("car_engine", {0, 0, 0});

// Game loop
while (running) {
  // Update sound position and velocity
  carPos.x += carVelocity.x * dt;
  audio.SetVoicePosition(car, carPos);
  audio.SetVoiceVelocity(car, carVelocity);
  
  audio.Update(dt); // Doppler pitch calculated automatically
}
```

---

## Randomization

Random pitch/volume variation and playlist support for audio variety.

### Volume/Pitch Variation

Events defined in JSON or programmatically can specify min/max ranges:

```json
{
  "name": "footstep",
  "path": "audio/footstep.wav",
  "volumeMin": 0.8,
  "volumeMax": 1.0,
  "pitchMin": 0.9,
  "pitchMax": 1.1
}
```

Each time the event plays, a random value within the range is selected.

### Playlist Mode

Events can have multiple sounds with different playback modes:

| Mode | Description |
|------|-------------|
| `Single` | Default - plays the single `path` sound |
| `Sequential` | Plays sounds in order, loops back |
| `Shuffle` | Randomizes order, plays each once, reshuffles |
| `Random` | Picks a random sound each time |

```json
{
  "name": "gunshot",
  "sounds": ["gun1.wav", "gun2.wav", "gun3.wav"],
  "playlistMode": "Shuffle",
  "volumeMin": 0.9,
  "volumeMax": 1.0,
  "pitchMin": 0.95,
  "pitchMax": 1.05
}
```

---

## Markers / Cues

Trigger callbacks at specific audio positions for synchronization.

### Core API

| Method | Description |
|--------|-------------|
| `void AddMarker(VoiceID id, float time, const string& name, function<void()> callback)` | Add a timed callback |
| `void RemoveMarker(VoiceID id, const string& name)` | Remove marker by name |
| `void ClearMarkers(VoiceID id)` | Remove all markers from voice |

### Example

```cpp
VoiceID music = audio.PlayEvent("background_music", {0, 0, 0});

// Add markers at specific times
audio.AddMarker(music, 1.5f, "beat1", []{ 
  std::cout << "Beat 1 hit!" << std::endl;
});
audio.AddMarker(music, 3.0f, "drop", []{ 
  SpawnExplosion(); 
});
audio.AddMarker(music, 10.0f, "ending", []{ 
  StartFadeOut(); 
});

// Game loop - markers fire automatically
while (running) {
  audio.Update(dt);
}
```

---

## Interactive Music

Beat-synchronized segment changes, stingers, and crossfades.

### Core API (via MusicManager)

```cpp
MusicManager& music = audio.GetMusicManager();
```

| Method | Description |
|--------|-------------|
| `void SetBPM(float bpm)` | Set beats per minute (default: 120) |
| `void SetBeatsPerBar(int beats)` | Set beats per bar (default: 4) |
| `void PlaySegment(const string& segment, float fadeTime)` | Play segment with crossfade |
| `void QueueSegment(const string& segment, TransitionSync sync, float fadeTime)` | Queue segment for next beat/bar |
| `void PlayStinger(const string& stinger, float volume)` | Play one-shot overlay |
| `void Stop(float fadeTime)` | Stop music with fade |
| `void SetBeatCallback(function<void(int)> cb)` | Callback on each beat |

### TransitionSync Options

| Mode | Description |
|------|-------------|
| `Immediate` | Transition now |
| `NextBeat` | Wait for next beat |
| `NextBar` | Wait for next bar |

### Example

```cpp
auto& music = audio.GetMusicManager();
music.SetBPM(120.0f);

// Play exploration music
music.PlaySegment("exploration", 0.5f);

// Queue combat music for next bar
music.QueueSegment("combat", TransitionSync::NextBar, 1.0f);

// Play victory stinger over music
music.PlayStinger("victory_fanfare", 0.8f);

// Beat callback
music.SetBeatCallback([](int beat) {
  std::cout << "Beat: " << beat << std::endl;
});
```

---

## Parameters

Global parameters that can drive audio behavior.

| Method | Description |
|--------|-------------|
| `void SetGlobalParameter(const std::string& name, float value)` | Set a global parameter. |
| `Parameter* GetParam(const std::string& name)` | Get a parameter to read or bind. |

**Parameter class:**
| Method | Description |
|--------|-------------|
| `float Get() const` | Get current value. |
| `void Set(float v)` | Set value (triggers callbacks). |
| `void Bind(std::function<void(float)> cb)` | Bind a callback for value changes. |

---

## RTPC Curves

Map parameter values to effect outputs through interpolated curves.

### CurvePoint Struct

```cpp
struct CurvePoint { float x, y; };  // Input → Output
```

### RTPCCurve Class

| Method | Description |
|--------|-------------|
| `void AddPoint(float x, float y)` | Add control point |
| `float Evaluate(float input) const` | Get interpolated output |
| `void Clear()` | Remove all points |

### API

| Method | Description |
|--------|-------------|
| `void BindRTPC(const string& param, const RTPCCurve& curve, function<void(float)> cb)` | Bind parameter to curve |
| `void UnbindRTPC(const string& param)` | Remove RTPC binding |

### Example

```cpp
// Map engine RPM (normalized 0-1) to pitch (0.8-2.0)
RTPCCurve enginePitch;
enginePitch.AddPoint(0.0f, 0.8f);   // Idle: low pitch
enginePitch.AddPoint(0.5f, 1.2f);   // Mid: normal
enginePitch.AddPoint(1.0f, 2.0f);   // Max: high pitch

audio.BindRTPC("engine_rpm", enginePitch, [&](float pitch) {
  audio.Engine().setRelativePlaySpeed(engineHandle, pitch);
});

// Update parameter → curve evaluates → callback fires
audio.SetGlobalParameter("engine_rpm", 0.75f);  // ~1.6 pitch
```

---

## Profiler

Query real-time audio engine statistics.

### AudioStats Struct

| Field | Type | Description |
|-------|------|-------------|
| `activeVoices` | `uint32_t` | Currently playing (real) voices |
| `virtualVoices` | `uint32_t` | Virtualized voices |
| `totalVoices` | `uint32_t` | Total tracked voices |
| `maxVoices` | `uint32_t` | Maximum voice limit |
| `cpuUsage` | `float` | Estimated CPU usage (0-100%) |
| `memoryUsed` | `size_t` | Estimated memory in bytes |
| `sampleRate` | `uint32_t` | Engine sample rate |
| `bufferSize` | `uint32_t` | Buffer size in samples |
| `channels` | `uint32_t` | Output channels |

### API

| Method | Description |
|--------|-------------|
| `AudioStats GetStats() const` | Get current engine statistics |

### Example

```cpp
AudioStats stats = audio.GetStats();
std::cout << "Active: " << stats.activeVoices << "/" << stats.maxVoices << "\n";
std::cout << "CPU: " << stats.cpuUsage << "%\n";
std::cout << "Sample rate: " << stats.sampleRate << " Hz\n";
```

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
#include "Orpheus.h"
using namespace Orpheus;

int main() {
  AudioManager audio;
  audio.Init();

  // Register and play event
  audio.LoadEventsFromFile("assets/events.json");
  audio.PlayEvent("explosion");

  // Set up 3D listener
  ListenerID listener = audio.CreateListener();
  audio.SetListenerPosition(listener, 0, 0, 0);

  // Main loop
  while (running) {
    audio.SetListenerPosition(listener, playerX, playerY, playerZ);
    audio.Update(deltaTime);
  }

  audio.Shutdown();
  return 0;
}
```

---

## Error Handling

Orpheus uses a `Result<T>` pattern for explicit error handling without exceptions.

### Core Types

| Type | Description |
|------|-------------|
| `ErrorCode` | Enum of error categories (FileNotFound, EventNotFound, etc.) |
| `Error` | Holds an `ErrorCode` and optional message |
| `Result<T>` | Either a value of type `T` or an `Error` |
| `Status` | Alias for `Result<void>` for functions that return nothing |

### Usage

```cpp
// Check initialization
auto initResult = audio.Init();
if (initResult.IsError()) {
  std::cerr << initResult.GetError().What() << "\n";
  return -1;
}

// Check sound playback
auto voiceResult = audio.PlayEvent("explosion");
if (voiceResult.IsError()) {
  // Handle error
} else {
  VoiceID id = voiceResult.Value();
}

// Get bus with error handling
auto busResult = audio.GetBus("Music");
if (busResult) {  // Implicit bool conversion
  busResult.Value()->SetVolume(0.5f);
}

// Use ValueOr for defaults
VoiceID id = audio.PlayEvent("music").ValueOr(0);
```

### ErrorCode Values

| Code | Description |
|------|-------------|
| `Ok` | No error |
| `EngineInitFailed` | Audio engine initialization failed |
| `FileNotFound` | Audio file not found |
| `EventNotFound` | Named event not registered |
| `BusNotFound` | Bus doesn't exist |
| `SnapshotNotFound` | Snapshot doesn't exist |
| `ReverbBusNotFound` | Reverb bus doesn't exist |
| `JsonParseError` | Invalid JSON format |
| `PlaybackFailed` | Sound playback failed |
| `VoiceAllocationFailed` | Voice pool exhausted |

---

## Logging

Configurable logging with callbacks for integration with your game's logging system.

### Log Levels

| Level | Description |
|-------|-------------|
| `Debug` | Detailed debugging info |
| `Info` | General information |
| `Warn` | Potential issues |
| `Error` | Errors that need attention |
| `None` | Disable all logging |

### Configuration

```cpp
// Set minimum log level (default: Info)
Orpheus::GetLogger().SetMinLevel(Orpheus::LogLevel::Debug);

// Set custom callback
Orpheus::GetLogger().SetCallback([](Orpheus::LogLevel level, 
                                     const std::string& msg) {
  myGameLog(level, msg);  // Forward to your system
});

// Clear callback (revert to stderr)
Orpheus::GetLogger().ClearCallback();
```

### Logging Macros

```cpp
ORPHEUS_DEBUG("Initializing engine");
ORPHEUS_INFO("Loaded " << count << " events");
ORPHEUS_WARN("Event not found: " << name);
ORPHEUS_ERROR("Failed to load: " << path);
```

---

## Unit Testing

Orpheus includes a comprehensive test suite using [Catch2](https://github.com/catchorg/Catch2).

### Running Tests

```bash
# Build with tests (default)
cmake -DORPHEUS_BUILD_TESTS=ON ..
cmake --build .

# Run tests
./orpheus_tests

# Run with verbose output
./orpheus_tests --success

# Run specific test cases
./orpheus_tests "[Error]"
./orpheus_tests "[VoicePool]"
```

### Test Coverage

| Module | Test File |
|--------|----------|
| Error, Result, Status | `test_error.cpp` |
| Types, Vector3 | `test_types.cpp` |
| Voice, VoiceState | `test_voice.cpp` |
| Snapshot, BusState | `test_snapshot.cpp` |
| Parameter | `test_parameter.cpp` |
| SoundBank, EventDescriptor | `test_soundbank.cpp` |
| VoicePool | `test_voicepool.cpp` |
| Logger | `test_log.cpp` |

---

## Thread Safety

Orpheus follows a **single-threaded ownership** model for most APIs, with explicit thread-safe entry points where needed.

### Threading Model

| Category | Thread Safety | Notes |
|----------|---------------|-------|
| **SoLoud Engine** | ✅ Internal locks | Audio mixing runs on a separate thread; SoLoud handles synchronization |
| **Logger** | ✅ Thread-safe | All logging methods protected by `m_Mutex` |
| **Parameters** | ✅ Thread-safe | `SetGlobalParameter`/`GetParam` protected by `m_ParamMutex` |
| **All other APIs** | ❌ Main thread only | Must be called from the same thread that called `Init()` |

### Per-Class Guarantees

#### AudioManager
| Method | Thread Safe | Notes |
|--------|-------------|-------|
| `Init()`, `Shutdown()` | ❌ | Call from main thread only |
| `Update(dt)` | ❌ | Call once per frame from main thread |
| `SetGlobalParameter()` | ✅ | Protected by mutex |
| `GetParam()` | ✅ | Protected by mutex |
| All other methods | ❌ | Not thread-safe |

#### Logger
| Method | Thread Safe | Notes |
|--------|-------------|-------|
| `Log()` | ✅ | All levels use mutex |
| `SetMinLevel()` | ✅ | Protected by mutex |
| `SetCallback()` | ✅ | Protected by mutex |

#### VoicePool, Bus, Zones, Snapshots
All methods are **NOT thread-safe**. Access only from the main thread.

### Recommended Usage Pattern

```cpp
// Main thread only - all audio operations
void GameLoop() {
    audio.Update(dt);
    audio.PlayEvent("explosion", pos);
    audio.SetListenerPosition(listener, pos);
}

// Safe from any thread - logging and parameters
void AnyThread() {
    ORPHEUS_INFO("Event triggered");
    audio.SetGlobalParameter("intensity", 0.8f);
}
```

### Why Not Full Thread Safety?

1. **Performance**: Mutexes on hot paths (playback, voice pool) would hurt performance
2. **Simplicity**: Audio typically runs on a single game thread
3. **SoLoud Design**: Underlying engine uses internal locks for audio thread, expects API calls from one thread

> [!WARNING]
> Calling non-thread-safe methods from multiple threads causes **undefined behavior** including crashes, corrupted state, and audio glitches.

---

## Benchmarks

Performance testing for voice management with Google Benchmark.

### Build & Run

```bash
cmake -B build -DORPHEUS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target orpheus_benchmarks
./build/orpheus_benchmarks
```

### Available Benchmarks

| Benchmark | Description |
|-----------|-------------|
| `BM_VoicePool_AllocateVoice` | Voice allocation speed |
| `BM_VoicePool_Update` | Per-frame update with N voices |
| `BM_VoicePool_VoiceStealing` | Priority-based voice stealing |
| `BM_Voice_UpdateAudibility` | Distance/audibility calculation |
| `BM_VoicePool_ChurnPattern` | Rapid allocate/deallocate cycle |

---

## Fuzzing

Fuzz testing for JSON parsing with libFuzzer.

### Build & Run (Clang only)

```bash
cmake -B build -DORPHEUS_BUILD_FUZZER=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build build
./build/fuzz_json_parser fuzz/corpus/ -max_total_time=60
```

### Fuzz Targets

| Target | Tests |
|--------|-------|
| `fuzz_json_parser` | SoundBank JSON parsing robustness |

---

## Code Coverage

Track test coverage with gcov (GCC) or llvm-cov (Clang).

### Prerequisites (Linux)

```bash
# For GCC coverage
sudo apt-get install lcov

# For Clang coverage
sudo apt-get install llvm
```

### Build & Generate Report

```bash
# With GCC (requires lcov)
cmake -B build -DORPHEUS_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/orpheus_tests
cmake --build build --target coverage
open build/coverage/html/index.html

# With Clang
cmake -B build -DORPHEUS_ENABLE_COVERAGE=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build build
LLVM_PROFILE_FILE="default.profraw" ./build/orpheus_tests
cmake --build build --target coverage
```
