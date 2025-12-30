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

  // Combat zone at position (80, 0, 0) - active from 55-105 units
  // Gap between 45-55 where music resets to normal
  audio.addMixZone("arena", "Combat", {80.0f, 0.0f, 0.0f}, 10.0f, 25.0f, 200);

  std::cout << "\nMix Zones created:\n";
  std::cout << "  - Cave at (30, 0, 0), range 15-45\n";
  std::cout << "  - Arena at (80, 0, 0), range 55-105\n";
  std::cout << "  - Gap at 45-55 where music resets\n";

  // Set up zone enter/exit callbacks
  audio.setZoneEnterCallback([](const std::string &zone) {
    std::cout << ">>> ENTERED zone: " << zone << "\n";
  });
  audio.setZoneExitCallback([](const std::string &zone) {
    std::cout << "<<< EXITED zone: " << zone << "\n";
  });

  // ============ SIMULATE PLAYER MOVEMENT ============
  std::cout << "\nSimulating player walking through zones...\n";

  float playerX = 0.0f;
  for (int i = 0; i < 500; ++i) {
    // Move player to the right over time
    if (i < 400) {
      playerX = (float)i * 0.2f; // 0 to 80 units
    } else {
      playerX = 80.0f - (float)(i - 400) * 0.2f; // Back to 60
    }

    audio.setListenerPosition(listener, playerX, 0.0f, 0.0f);
    audio.update(1.0f / 60.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

    // Print position and active zone periodically
    if (i % 50 == 0) {
      std::cout << "Position: (" << playerX << ", 0, 0) - Zone: "
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
