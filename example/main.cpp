#include "../GoldenLyre.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  // Create the audio manager
  AudioManager audio;
  if (!audio.init()) {
    std::cerr << "Failed to initialize AudioManager\n";
    return -1;
  }

  std::cout << "GoldenLyre Audio Engine initialized!\n";

  // Configure voice pool
  audio.setMaxVoices(8);
  audio.setStealBehavior(StealBehavior::Quietest);

  // Register music event
  EventDescriptor musicEvent;
  musicEvent.name = "music";
  musicEvent.path = "assets/raw/mellotrix_doodle.wav";
  musicEvent.bus = "Music";
  musicEvent.volumeMin = 0.7f;
  musicEvent.stream = true;
  musicEvent.priority = 255;
  audio.registerEvent(musicEvent);
  std::cout << "Registered 'music' event\n";

  // Register a waterfall ambient event (using flute as watery ambient sound)
  EventDescriptor waterfallEvent;
  waterfallEvent.name = "waterfall";
  waterfallEvent.path =
      "assets/raw/waterfall.wav"; // Different sound to avoid interference
  waterfallEvent.bus = "SFX";
  waterfallEvent.volumeMin = 0.6f;
  waterfallEvent.stream = false; // Short file, no need to stream
  audio.registerEvent(waterfallEvent);

  // underwater
  EventDescriptor underWaterEvent;
  underWaterEvent.name = "underwater";
  underWaterEvent.path =
      "assets/raw/underwater.wav"; // Different sound to avoid interference
  underWaterEvent.bus = "SFX";
  underWaterEvent.volumeMin = 0.6f;
  underWaterEvent.stream = false; // Short file, no need to stream
  audio.registerEvent(underWaterEvent);

  // Create listener at origin
  ListenerID listener = audio.createListener();
  audio.setListenerPosition(listener, 0.0f, 0.0f, 0.0f);

  // Play background music
  audio.playEvent("music");
  std::cout << "Playing music...\n";

  // ============ CREATE SNAPSHOTS ============
  // Cave snapshot - quieter music
  audio.createSnapshot("Cave");
  audio.setSnapshotBusVolume("Cave", "Music", 0.4f);

  // Combat snapshot - louder SFX, quieter music
  audio.createSnapshot("Combat");
  audio.setSnapshotBusVolume("Combat", "Music", 0.2f);
  audio.setSnapshotBusVolume("Combat", "SFX", 1.2f);

  // ============ CREATE MIX ZONES ============
  // Cave zone at position (30, 0, 0) - active from 15-45 units
  audio.addMixZone("cave", "Cave", {30.0f, 0.0f, 0.0f}, 5.0f, 15.0f, 100);

  // Combat zone at position (100, 0, 0) - active from 75-125 units
  // Larger gap between 45-75 where music resets to normal
  audio.addMixZone("arena", "Combat", {100.0f, 0.0f, 0.0f}, 10.0f, 25.0f, 200);

  std::cout << "\nMix Zones created:\n";
  std::cout << "  - Cave at (30, 0, 0), range 15-45\n";
  std::cout << "  - Arena at (100, 0, 0), range 75-125\n";
  std::cout << "  - Gap at 45-75 where music resets (~30 units)\n";

  // ============ ZONE-TRIGGERED SNAPSHOTS ============
  // Create an "Underwater" snapshot for the waterfall area
  audio.createSnapshot("Underwater");
  audio.setSnapshotBusVolume("Underwater", "Music", 0.3f);
  audio.setSnapshotBusVolume("Underwater", "SFX", 0.5f);

  // Create AudioZone with snapshot binding in the gap between cave and arena
  // When player enters this zone, "Underwater" snapshot is automatically
  // applied When player exits, bus volumes reset to default
  audio.addAudioZone("waterfall", {60.0f, 0.0f, 0.0f}, 5.0f, 10.0f,
                     "Underwater",
                     0.5f,  // fade in time
                     1.0f); // fade out time

  std::cout << "\nZone-Triggered Snapshot created:\n";
  std::cout << "  - Waterfall at (60, 0, 0), range 50-70\n";
  std::cout << "  - Triggers 'Underwater' snapshot on enter\n";

  // Set up zone enter/exit callbacks
  audio.setZoneEnterCallback([](const std::string &zone) {
    std::cout << ">>> ENTERED zone: " << zone << "\n";
  });
  audio.setZoneExitCallback([](const std::string &zone) {
    std::cout << "<<< EXITED zone: " << zone << "\n";
  });

  // ============ SIMULATE PLAYER MOVEMENT ============
  std::cout << "\nSimulating player walking through zones (~1 minute)...\n";
  std::cout << "Timeline: Start -> Cave -> Gap -> Waterfall -> Gap -> Arena -> "
               "End\n\n";

  float playerX = 0.0f;
  const float speed = 0.05f;    // Slower movement for longer demo
  const int totalFrames = 3600; // ~60 seconds at 60fps

  for (int i = 0; i < totalFrames; ++i) {
    // Move player to the right, then back
    if (i < 3000) {
      playerX = (float)i * speed; // 0 to 150 units over 50 seconds
    } else {
      playerX = 150.0f - (float)(i - 3000) * speed; // Back towards 120
    }

    audio.setListenerPosition(listener, playerX, 0.0f, 0.0f);
    audio.update(1.0f / 60.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

    // Print position and elapsed time every ~5 seconds
    if (i % 300 == 0) {
      float elapsed = (float)i / 60.0f;
      std::cout << "[" << std::fixed << std::setprecision(0) << elapsed << "s] "
                << "Position: " << std::setprecision(1) << playerX
                << " - Zone: "
                << (audio.getActiveMixZone().empty() ? "[none]"
                                                     : audio.getActiveMixZone())
                << "\n";
    }
  }

  std::cout << "\nShutting down...\n";
  audio.shutdown();

  std::cout << "Done!\n";
  return 0;
}
