#include "../Orpheus.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace Orpheus;

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  // Create the audio manager
  AudioManager audio;
  auto initResult = audio.Init();
  if (initResult.IsError()) {
    std::cerr << "Failed to initialize AudioManager: "
              << initResult.GetError().What() << "\n";
    return -1;
  }

  std::cout << "Orpheus Audio Engine initialized!\n";

  // Configure voice pool
  audio.SetMaxVoices(8);
  audio.SetStealBehavior(StealBehavior::Quietest);

  // Register music event
  EventDescriptor musicEvent;
  musicEvent.name = "music";
  musicEvent.path = "assets/raw/mellotrix_doodle.wav";
  musicEvent.bus = "Music";
  musicEvent.volumeMin = 0.7f;
  musicEvent.stream = true;
  musicEvent.priority = 255;
  audio.RegisterEvent(musicEvent);
  std::cout << "Registered 'music' event\n";

  // Register a waterfall ambient event
  EventDescriptor waterfallEvent;
  waterfallEvent.name = "waterfall";
  waterfallEvent.path = "assets/raw/waterfall.wav";
  waterfallEvent.bus = "SFX";
  waterfallEvent.volumeMin = 0.6f;
  waterfallEvent.stream = false;
  audio.RegisterEvent(waterfallEvent);

  // underwater
  EventDescriptor underWaterEvent;
  underWaterEvent.name = "underwater";
  underWaterEvent.path = "assets/raw/underwater.wav";
  underWaterEvent.bus = "SFX";
  underWaterEvent.volumeMin = 0.6f;
  underWaterEvent.stream = false;
  audio.RegisterEvent(underWaterEvent);

  // Create listener at origin
  ListenerID listener = audio.CreateListener();
  audio.SetListenerPosition(listener, 0.0f, 0.0f, 0.0f);

  // Play background music
  audio.PlayEvent("music");
  std::cout << "Playing music...\n";

  // ============ CREATE SNAPSHOTS ============
  // Cave snapshot - quieter music
  audio.CreateSnapshot("Cave");
  audio.SetSnapshotBusVolume("Cave", "Music", 0.4f);

  // Combat snapshot - louder SFX, quieter music
  audio.CreateSnapshot("Combat");
  audio.SetSnapshotBusVolume("Combat", "Music", 0.2f);
  audio.SetSnapshotBusVolume("Combat", "SFX", 1.2f);

  // ============ CREATE MIX ZONES ============
  // Cave zone at position (30, 0, 0) - active from 15-45 units
  audio.AddMixZone("cave", "Cave", {30.0f, 0.0f, 0.0f}, 5.0f, 15.0f, 100);

  // Combat zone at position (100, 0, 0) - active from 75-125 units
  audio.AddMixZone("arena", "Combat", {100.0f, 0.0f, 0.0f}, 10.0f, 25.0f, 200);

  std::cout << "\nMix Zones created:\n";
  std::cout << "  - Cave at (30, 0, 0), range 15-45\n";
  std::cout << "  - Arena at (100, 0, 0), range 75-125\n";
  std::cout << "  - Gap at 45-75 where music resets (~30 units)\n";

  // ============ ZONE-TRIGGERED SNAPSHOTS ============
  // Create an "Underwater" snapshot for the waterfall area
  audio.CreateSnapshot("Underwater");
  audio.SetSnapshotBusVolume("Underwater", "Music", 0.3f);
  audio.SetSnapshotBusVolume("Underwater", "SFX", 0.5f);

  // Create AudioZone with snapshot binding in the gap between cave and arena
  audio.AddAudioZone("waterfall", {60.0f, 0.0f, 0.0f}, 5.0f, 10.0f,
                     "Underwater", 0.5f, 1.0f);

  std::cout << "\nZone-Triggered Snapshot created:\n";
  std::cout << "  - Waterfall at (60, 0, 0), range 50-70\n";
  std::cout << "  - Triggers 'Underwater' snapshot on enter\n";

  // ============ REVERB BUSES ============
  audio.CreateReverbBus("CaveReverb", ReverbPreset::Cave);
  audio.CreateReverbBus("HallReverb", ReverbPreset::Hall);

  // Create reverb zones
  audio.AddReverbZone("cave_reverb", "CaveReverb", {30.0f, 0.0f, 0.0f}, 5.0f,
                      20.0f, 150);
  audio.AddReverbZone("arena_reverb", "HallReverb", {100.0f, 0.0f, 0.0f}, 10.0f,
                      30.0f, 100);

  std::cout << "\nReverb Buses created:\n";
  std::cout << "  - CaveReverb (Cave preset) at position (30, 0, 0)\n";
  std::cout << "  - HallReverb (Hall preset) at position (100, 0, 0)\n";
  std::cout << "  - Reverb wet level fades based on distance to zone center\n";

  // ============ OCCLUSION DEMO ============
  const float wallPosition = 45.0f;
  const float musicSourcePos = 0.0f;

  audio.SetOcclusionQueryCallback(
      [wallPosition,
       musicSourcePos](const Vector3 &source,
                       const Vector3 &listener) -> std::vector<OcclusionHit> {
        std::vector<OcclusionHit> hits;

        float srcX = source.x;
        float listenerX = listener.x;

        if ((srcX < wallPosition && listenerX > wallPosition) ||
            (srcX > wallPosition && listenerX < wallPosition)) {
          hits.push_back(OcclusionHit{"Concrete", 0.3f});
        }

        return hits;
      });

  audio.SetOcclusionEnabled(true);
  audio.SetOcclusionThreshold(0.7f);
  audio.SetOcclusionSmoothingTime(0.15f);

  std::cout << "\nOcclusion enabled:\n";
  std::cout << "  - Simulated concrete wall at x=45\n";
  std::cout << "  - Music will be muffled when listener crosses the wall\n";

  // Set up zone enter/exit callbacks
  audio.SetZoneEnterCallback([](const std::string &zone) {
    std::cout << ">>> ENTERED zone: " << zone << "\n";
  });
  audio.SetZoneExitCallback([](const std::string &zone) {
    std::cout << "<<< EXITED zone: " << zone << "\n";
  });

  // ============ SIMULATE PLAYER MOVEMENT ============
  std::cout << "\nSimulating player walking through zones (~1 minute)...\n";
  std::cout << "Timeline: Start -> Cave -> Gap -> Waterfall -> Gap -> Arena -> "
               "End\n\n";

  float playerX = 0.0f;
  const float speed = 0.05f;
  const int totalFrames = 3600;

  for (int i = 0; i < totalFrames; ++i) {
    if (i < 3000) {
      playerX = (float)i * speed;
    } else {
      playerX = 150.0f - (float)(i - 3000) * speed;
    }

    audio.SetListenerPosition(listener, playerX, 0.0f, 0.0f);
    audio.Update(1.0f / 60.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

    if (i % 300 == 0) {
      float elapsed = (float)i / 60.0f;
      std::cout << "[" << std::fixed << std::setprecision(0) << elapsed << "s] "
                << "Position: " << std::setprecision(1) << playerX
                << " - Zone: "
                << (audio.GetActiveMixZone().empty() ? "[none]"
                                                     : audio.GetActiveMixZone())
                << "\n";
    }
  }

  std::cout << "\nShutting down...\n";
  audio.Shutdown();

  std::cout << "Done!\n";
  return 0;
}
