#include <benchmark/benchmark.h>

#include "../include/VoicePool.h"

using namespace Orpheus;

// =============================================================================
// VoicePool Benchmarks
// =============================================================================

static void BM_VoicePool_AllocateVoice(benchmark::State &state) {
  VoicePool pool(static_cast<uint32_t>(state.range(0)));

  for (auto _ : state) {
    Voice *voice = pool.AllocateVoice("test_event", 128, {0, 0, 0},
                                      DistanceSettings{.maxDistance = 100.0f});
    benchmark::DoNotOptimize(voice);
    pool.StopVoice(voice); // Reset for next iteration
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VoicePool_AllocateVoice)->Arg(32)->Arg(64)->Arg(128)->Arg(256);

static void BM_VoicePool_AllocateAndMakeReal(benchmark::State &state) {
  VoicePool pool(static_cast<uint32_t>(state.range(0)));

  for (auto _ : state) {
    Voice *voice = pool.AllocateVoice("test_event", 128, {0, 0, 0},
                                      DistanceSettings{.maxDistance = 100.0f});
    bool success = pool.MakeReal(voice);
    benchmark::DoNotOptimize(success);
    pool.StopVoice(voice);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VoicePool_AllocateAndMakeReal)->Arg(32)->Arg(64)->Arg(128);

static void BM_VoicePool_Update(benchmark::State &state) {
  VoicePool pool(32);
  const int voiceCount = static_cast<int>(state.range(0));

  // Pre-allocate voices
  for (int i = 0; i < voiceCount; ++i) {
    Voice *v = pool.AllocateVoice("event_" + std::to_string(i), 128,
                                  {static_cast<float>(i), 0, 0},
                                  DistanceSettings{.maxDistance = 50.0f});
    pool.MakeReal(v);
  }

  Vector3 listenerPos{0, 0, 0};
  float dt = 0.016f; // ~60 FPS

  for (auto _ : state) {
    pool.Update(dt, listenerPos);
    benchmark::DoNotOptimize(pool.GetRealVoiceCount());
  }

  state.SetItemsProcessed(state.iterations() * voiceCount);
}
BENCHMARK(BM_VoicePool_Update)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

static void BM_VoicePool_VoiceStealing(benchmark::State &state) {
  const uint32_t maxVoices = 32;
  VoicePool pool(maxVoices);
  pool.SetStealBehavior(StealBehavior::Quietest);

  // Fill pool to capacity with real voices
  for (uint32_t i = 0; i < maxVoices; ++i) {
    Voice *v = pool.AllocateVoice("fill_" + std::to_string(i), 64,
                                  {static_cast<float>(i * 10), 0, 0},
                                  DistanceSettings{.maxDistance = 100.0f});
    v->audibility = 0.1f + (static_cast<float>(i) / maxVoices) * 0.5f;
    pool.MakeReal(v);
  }

  for (auto _ : state) {
    // Allocate high-priority voice that triggers stealing
    Voice *newVoice =
        pool.AllocateVoice("high_priority", 255, {0, 0, 0},
                           DistanceSettings{.maxDistance = 50.0f});
    newVoice->audibility = 1.0f;
    bool success = pool.MakeReal(newVoice);
    benchmark::DoNotOptimize(success);

    // Make the stolen voice real again to reset state
    pool.MakeVirtual(newVoice);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VoicePool_VoiceStealing);

// =============================================================================
// Voice State Benchmarks
// =============================================================================

static void BM_Voice_UpdateAudibility(benchmark::State &state) {
  Voice voice;
  voice.id = 1;
  voice.position = {100.0f, 0.0f, 0.0f};
  voice.distanceSettings.maxDistance = 200.0f;
  voice.state = VoiceState::Real;

  Vector3 listenerPos{0, 0, 0};

  for (auto _ : state) {
    voice.UpdateAudibility(listenerPos);
    benchmark::DoNotOptimize(voice.audibility);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Voice_UpdateAudibility);

static void BM_Voice_UpdateAudibility_Batch(benchmark::State &state) {
  const int count = static_cast<int>(state.range(0));
  std::vector<Voice> voices(count);

  for (int i = 0; i < count; ++i) {
    voices[i].id = i;
    voices[i].position = {static_cast<float>(i * 10), 0, 0};
    voices[i].distanceSettings.maxDistance = 200.0f;
    voices[i].state = VoiceState::Real;
  }

  Vector3 listenerPos{50.0f, 0, 0};

  for (auto _ : state) {
    for (auto &v : voices) {
      v.UpdateAudibility(listenerPos);
    }
    benchmark::DoNotOptimize(voices[0].audibility);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_Voice_UpdateAudibility_Batch)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256);

// =============================================================================
// Memory Allocation Pattern Benchmarks
// =============================================================================

static void BM_VoicePool_ChurnPattern(benchmark::State &state) {
  // Simulates typical game pattern: rapid allocate/deallocate
  VoicePool pool(32);
  std::vector<Voice *> activeVoices;
  activeVoices.reserve(16);

  for (auto _ : state) {
    // Allocate some voices
    for (int i = 0; i < 8; ++i) {
      Voice *v = pool.AllocateVoice("churn", 128, {0, 0, 0},
                                    DistanceSettings{.maxDistance = 50.0f});
      pool.MakeReal(v);
      activeVoices.push_back(v);
    }

    // Stop half of them
    for (size_t i = 0; i < activeVoices.size() / 2; ++i) {
      pool.StopVoice(activeVoices[i]);
    }
    activeVoices.clear();
  }

  state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_VoicePool_ChurnPattern);

// =============================================================================
// Main
// =============================================================================

BENCHMARK_MAIN();
