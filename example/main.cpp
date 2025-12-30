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
  audio.setMaxVoices(8); // Only 8 simultaneous real voices
  audio.setStealBehavior(StealBehavior::Quietest);
  std::cout << "Voice pool: max " << audio.getMaxVoices() << " real voices\n";

  // Register events with different priorities
  const std::string explosionJson = R"({
    "name": "explosion",
    "sound": "assets/raw/explosion.wav",
    "bus": "SFX",
    "volume": [0.8, 1.0],
    "priority": 200,
    "maxDistance": 50.0,
    "parameters": {
      "distance": "attenuation"
    }
  })";

  if (audio.registerEvent(explosionJson)) {
    std::cout << "Registered 'explosion' event (priority 200)\n";
  }

  // Music event with high priority (never stolen)
  EventDescriptor musicEvent;
  musicEvent.name = "music";
  musicEvent.path = "assets/raw/mellotrix_doodle.wav";
  musicEvent.bus = "Music";
  musicEvent.volumeMin = 0.7f;
  musicEvent.volumeMax = 1.0f;
  musicEvent.stream = true;
  musicEvent.priority = 255; // Highest priority - never stolen

  audio.registerEvent(musicEvent);
  std::cout << "Registered 'music' event (priority 255)\n";

  // Create a listener
  ListenerID listener = audio.createListener();
  audio.setListenerPosition(listener, 0.0f, 0.0f, 0.0f);

  // Play music (uses voice pool)
  std::cout << "\nPlaying music...\n";
  VoiceID musicVoice = audio.playEvent("music");

  // Demonstrate voice limiting by spawning many sounds
  std::cout << "\nSpawning 20 explosions at various positions...\n";
  for (int i = 0; i < 20; i++) {
    // Spread explosions in a circle around listener
    float angle = (float)i * 0.314f;     // ~18 degrees apart
    float dist = 10.0f + (i % 5) * 5.0f; // 10-30 units away
    Vector3 pos{cosf(angle) * dist, 0.0f, sinf(angle) * dist};
    audio.playEvent("explosion", pos);
  }

  std::cout << "Real voices: " << audio.getRealVoiceCount()
            << " | Virtual: " << audio.getVirtualVoiceCount()
            << " | Total: " << audio.getActiveVoiceCount() << "\n";

  // Create snapshot for combat
  audio.createSnapshot("Combat");
  audio.setSnapshotBusVolume("Combat", "Music", 0.3f);
  audio.setSnapshotBusVolume("Combat", "SFX", 1.0f);

  // Simulate main loop (5 seconds)
  std::cout << "\nRunning audio update loop for 5 seconds...\n";
  for (int i = 0; i < 3000; ++i) {
    audio.update(1.0f / 60.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

    // Log voice counts periodically
    if (i % 600 == 0) {
      std::cout << "Frame " << i << " - Real: " << audio.getRealVoiceCount()
                << " Virtual: " << audio.getVirtualVoiceCount() << "\n";
    }

    // Move listener toward explosions at frame 120
    if (i == 1200) {
      std::cout << "Moving listener toward explosions...\n";
      audio.setListenerPosition(listener, 15.0f, 0.0f, 0.0f);
    }

    // Apply combat snapshot at frame 180
    if (i >= 1800 && i < 2400) {
      audio.applySnapshot("Combat", 0.5f);
    }

    // Reset at frame 240
    if (i == 2400) {
      audio.resetBusVolumes(0.5f);
    }
  }

  std::cout << "\nShutting down...\n";
  audio.shutdown();

  std::cout << "Done!\n";
  return 0;
}
