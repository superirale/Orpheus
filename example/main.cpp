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

  // Method 1: Register event from JSON string
  const std::string explosionJson = R"({
    "name": "explosion",
    "sound": "assets/raw/explosion.wav",
    "bus": "SFX",
    "volume": [0.8, 1.0],
    "pitch": [0.9, 1.1],
    "parameters": {
      "distance": "attenuation"
    }
  })";

  if (audio.registerEvent(explosionJson)) {
    std::cout << "Registered 'explosion' event from JSON\n";
  }

  // Method 2: Register event using EventDescriptor struct
  EventDescriptor testEvent;
  testEvent.name = "flute";
  testEvent.path = "assets/raw/flute.wav";
  testEvent.bus = "SFX";
  testEvent.volumeMin = 0.7f;
  testEvent.volumeMax = 1.0f;
  testEvent.pitchMin = 0.6f;
  testEvent.pitchMax = 1.1f;
  testEvent.stream = false;

  audio.registerEvent(testEvent);
  std::cout << "Registered 'test_sfx' event from struct\n";

  // Play events
  std::cout << "\nPlaying events...\n";
  AudioHandle h1 = audio.playEvent("explosion");
  AudioHandle h2 = audio.playEvent("flute");

  if (h1 == 0 && h2 == 0) {
    std::cout << "Note: No audio files found. Place .wav files in assets/ to "
                 "hear playback.\n";
  }

  AudioData audioData;
  audioData.listenerPos = {0.0f, 0.0f, 0.0f};
  audio.addAudioZone("assets/raw/explosion.wav", {10.0f, 10.0f, 0.0f}, 1.0f, 10.0f, false);

  // Simulate main loop (2 seconds)
  std::cout << "Running audio update loop for 2 seconds...\n";
  for (int i = 0; i < 1200; ++i) {
    audio.update(1.0f / 60.0f, audioData);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

    if (i >=900) {
       audioData.listenerPos = {10.0f, 10.0f, 0.0f};
    }

  }

  std::cout << "Shutting down...\n";
  audio.shutdown();

  std::cout << "Done!\n";
  return 0;
}
