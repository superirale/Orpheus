#include "../Orpheus.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

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
  std::cout << "Demonstrating Event Delays and Playlist Looping...\n\n";

  // 1. Register a Delayed Event
//   EventDescriptor delayedEvent;
//   delayedEvent.name = "delayed_blast";
//   delayedEvent.path = "assets/raw/waterfall.wav"; // Reuse existing asset
//   delayedEvent.bus = "SFX";
//   delayedEvent.volumeMin = 1.0f;
//   delayedEvent.startDelay = 2.0f; // Wait 2 seconds
//   audio.RegisterEvent(delayedEvent);

  // 2. Register a Random Looping Playlist (Ambience)
//   EventDescriptor randomAmbience;
//   randomAmbience.name = "random_ambience";
//   randomAmbience.bus = "SFX";
//   randomAmbience.volumeMin = 0.8f;
//   randomAmbience.playlistMode = PlaylistMode::Random;
//   randomAmbience.loopPlaylist = true; // Repeat forever
//   randomAmbience.interval = 1.5f;     // Wait 1.5s between sounds

//   // adding same files multiple times to simulate variety
//   randomAmbience.sounds.push_back("assets/raw/waterfall.wav");
//   randomAmbience.sounds.push_back("assets/raw/underwater.wav");

//   audio.RegisterEvent(randomAmbience);

  // 3. Register a Sequential Looping Playlist (Music Stems)
  EventDescriptor seqMusic;
  seqMusic.name = "seq_music";
  seqMusic.bus = "Music";
  seqMusic.volumeMin = 0.6f;
  seqMusic.playlistMode = PlaylistMode::Sequential;
  seqMusic.loopPlaylist = true; // Loop back to start
  seqMusic.interval = 15.0f;     // Short gap between stems

  // Simulate stems
  seqMusic.sounds.push_back("assets/raw/underwater.wav");
  seqMusic.sounds.push_back("assets/raw/waterfall.wav");

  audio.RegisterEvent(seqMusic);

  // ==================================================================================

  // Test 1: Delayed Start
//   std::cout
//       << "[0.0s] Triggering 'delayed_blast' (Should hear it at 2.0s)...\n";
//   audio.PlayEvent("delayed_blast");

//   // Test 2: Random Ambience
//   std::cout << "[0.0s] Starting 'random_ambience' (Random loop with 1.5s "
//                "interval)...\n";
//   audio.PlayEvent("random_ambience");

  // Test 3: Sequential Music
  std::cout
      << "[0.0s] Starting 'seq_music' (Sequence loop with 0.5s interval)...\n";
  audio.PlayEvent("seq_music");

  // Simulation Loop
  const int totalFrames = 60000; // 10 seconds at 60fps
  for (int i = 0; i < totalFrames; ++i) {
    audio.Update(1.0f / 60.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

    float elapsed = (float)i / 60.0f;
    if (i % 60 == 0) { // Every second
      std::cout << "[" << std::fixed << std::setprecision(1) << elapsed
                << "s] Update...\n";
    }
  }

  std::cout << "\nShutting down...\n";
  audio.Shutdown();
  return 0;
}
