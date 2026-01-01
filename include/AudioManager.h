/**
 * @file AudioManager.h
 * @brief Main interface for the Orpheus Audio Engine.
 *
 * AudioManager is the primary class for all audio operations including
 * initialization, event playback, 3D audio, zones, snapshots, and more.
 *
 * @see Orpheus.h for the single-include header.
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Error.h"
#include "Listener.h"
#include "OcclusionMaterial.h"
#include "OcclusionQuery.h"
#include "ReverbBus.h"
#include "SoundBank.h"
#include "Types.h"
#include "Voice.h"

namespace Orpheus {

// Forward declarations for types used in API
class Bus;
class Parameter;

/**
 * @brief Callback for zone entry events.
 * @param zoneName Name of the zone entered.
 */
using ZoneEnterCallback = std::function<void(const std::string &)>;

/**
 * @brief Callback for zone exit events.
 * @param zoneName Name of the zone exited.
 */
using ZoneExitCallback = std::function<void(const std::string &)>;

/**
 * @brief Main audio system manager.
 *
 * AudioManager provides a high-level interface to the Orpheus audio engine.
 * It handles initialization, event playback, 3D spatialization, zones,
 * snapshots, buses, reverb, and occlusion.
 *
 * @par Basic Usage:
 * @code
 * Orpheus::AudioManager audio;
 * if (!audio.Init()) {
 *     // Handle error
 * }
 *
 * // Register and play events
 * audio.RegisterEvent({"explosion", "sounds/explosion.wav", "SFX"});
 * audio.PlayEvent("explosion", {10.0f, 0.0f, 5.0f});
 *
 * // Main loop
 * while (running) {
 *     audio.Update(deltaTime);
 * }
 *
 * audio.Shutdown();
 * @endcode
 *
 * @note Call Init() before any other operations.
 * @note Call Update() each frame to process audio state.
 * @note Call Shutdown() before destruction.
 *
 * @par Thread Safety:
 * Most methods must be called from the main thread (the thread that called
 * Init()). The following methods are thread-safe:
 * - SetGlobalParameter() - Protected by internal mutex
 * - GetParam() - Protected by internal mutex
 *
 * @warning Calling non-thread-safe methods from multiple threads causes
 *          undefined behavior.
 *
 * @par ABI Stability:
 * This class uses the PIMPL idiom. Implementation details are hidden,
 * allowing internal changes without breaking binary compatibility.
 */
class AudioManager {
public:
  /**
   * @brief Default constructor.
   */
  AudioManager();

  /**
   * @brief Destructor (calls Shutdown if needed).
   */
  ~AudioManager();

  // Non-copyable
  AudioManager(const AudioManager &) = delete;
  AudioManager &operator=(const AudioManager &) = delete;

  // Movable
  AudioManager(AudioManager &&) noexcept;
  AudioManager &operator=(AudioManager &&) noexcept;

  /// @name Core Lifecycle
  /// @{

  /**
   * @brief Initialize the audio engine.
   * @return Status indicating success or error.
   */
  Status Init();

  /**
   * @brief Shutdown the audio engine.
   */
  void Shutdown();

  /**
   * @brief Update audio state. Call once per frame.
   * @param dt Delta time in seconds.
   */
  void Update(float dt);

  /// @}

  /// @name Event Playback
  /// @{

  /**
   * @brief Play a registered audio event.
   * @param name Event name.
   * @param position 3D position (default: origin).
   * @return Result containing VoiceID or error.
   */
  Result<VoiceID> PlayEvent(const std::string &name,
                            Vector3 position = {0, 0, 0});

  /**
   * @brief Play an event directly and return its handle.
   * @param name Event name.
   * @return Result containing AudioHandle or error.
   */
  Result<AudioHandle> PlayEventDirect(const std::string &name);

  /**
   * @brief Register an event from a descriptor.
   * @param ed Event descriptor.
   */
  void RegisterEvent(const EventDescriptor &ed);

  /**
   * @brief Register an event from a JSON string.
   * @param jsonString JSON event definition.
   * @return Status indicating success or error.
   */
  Status RegisterEvent(const std::string &jsonString);

  /**
   * @brief Load events from a JSON file.
   * @param jsonPath Path to JSON file.
   * @return Status indicating success or error.
   */
  Status LoadEventsFromFile(const std::string &jsonPath);

  /// @}

  /// @name Parameters
  /// @{

  /**
   * @brief Set a global parameter value.
   * @param name Parameter name.
   * @param value New value.
   */
  void SetGlobalParameter(const std::string &name, float value);

  /**
   * @brief Get a parameter by name.
   * @param name Parameter name.
   * @return Pointer to parameter, or nullptr if not found.
   */
  [[nodiscard]] Parameter *GetParam(const std::string &name);

  /// @}

  /// @name Audio Zones
  /// @{

  /**
   * @brief Add an audio zone.
   * @param eventName Event to play in the zone.
   * @param pos Zone center position.
   * @param inner Inner radius (full volume).
   * @param outer Outer radius (zero volume).
   */
  void AddAudioZone(const std::string &eventName, const Vector3 &pos,
                    float inner, float outer);

  /**
   * @brief Add an audio zone with snapshot.
   * @param eventName Event to play in the zone.
   * @param pos Zone center position.
   * @param inner Inner radius.
   * @param outer Outer radius.
   * @param snapshotName Snapshot to apply when active.
   * @param fadeIn Snapshot fade-in time.
   * @param fadeOut Snapshot fade-out time.
   */
  void AddAudioZone(const std::string &eventName, const Vector3 &pos,
                    float inner, float outer, const std::string &snapshotName,
                    float fadeIn = 0.5f, float fadeOut = 0.5f);

  /// @}

  /// @name Listener Management
  /// @{

  /**
   * @brief Create a new listener.
   * @return Unique listener ID.
   */
  ListenerID CreateListener();

  /**
   * @brief Destroy a listener.
   * @param id Listener ID.
   */
  void DestroyListener(ListenerID id);

  /**
   * @brief Set listener position.
   * @param id Listener ID.
   * @param pos New position.
   */
  void SetListenerPosition(ListenerID id, const Vector3 &pos);

  /**
   * @brief Set listener position (component form).
   * @param id Listener ID.
   * @param x X coordinate.
   * @param y Y coordinate.
   * @param z Z coordinate.
   */
  void SetListenerPosition(ListenerID id, float x, float y, float z);

  /**
   * @brief Set listener velocity (for Doppler).
   * @param id Listener ID.
   * @param vel Velocity vector.
   */
  void SetListenerVelocity(ListenerID id, const Vector3 &vel);

  /**
   * @brief Set listener orientation.
   * @param id Listener ID.
   * @param forward Forward direction.
   * @param up Up direction.
   */
  void SetListenerOrientation(ListenerID id, const Vector3 &forward,
                              const Vector3 &up);

  /// @}

  /// @name Bus API
  /// @{

  /**
   * @brief Create a new audio bus.
   * @param name Unique bus name.
   */
  void CreateBus(const std::string &name);

  /**
   * @brief Get a bus by name.
   * @param name Bus name.
   * @return Result containing shared_ptr to Bus or error.
   */
  [[nodiscard]] Result<std::shared_ptr<Bus>> GetBus(const std::string &name);

  /// @}

  /// @name Snapshot API
  /// @{

  /**
   * @brief Create a new snapshot.
   * @param name Unique snapshot name.
   */
  void CreateSnapshot(const std::string &name);

  /**
   * @brief Set a bus volume in a snapshot.
   * @param snap Snapshot name.
   * @param bus Bus name.
   * @param volume Target volume.
   */
  void SetSnapshotBusVolume(const std::string &snap, const std::string &bus,
                            float volume);

  /**
   * @brief Apply a snapshot.
   * @param name Snapshot name.
   * @param fadeSeconds Fade transition time.
   * @return Status indicating success or error.
   */
  Status ApplySnapshot(const std::string &name, float fadeSeconds = 0.3f);

  /**
   * @brief Reset all bus volumes to defaults.
   * @param fadeSeconds Fade transition time.
   */
  void ResetBusVolumes(float fadeSeconds = 0.3f);

  /**
   * @brief Reset a specific event's volume.
   * @param eventName Event name.
   * @param fadeSeconds Fade transition time.
   */
  void ResetEventVolume(const std::string &eventName, float fadeSeconds = 0.3f);

  /// @}

  /// @name Voice Pool API
  /// @{

  /**
   * @brief Set maximum concurrent voices.
   * @param maxReal Maximum real (hardware) voices.
   */
  void SetMaxVoices(uint32_t maxReal);

  /**
   * @brief Get maximum concurrent voices.
   * @return Maximum voice count.
   */
  [[nodiscard]] uint32_t GetMaxVoices() const;

  /**
   * @brief Set voice stealing behavior.
   * @param behavior Steal behavior.
   */
  void SetStealBehavior(StealBehavior behavior);

  /**
   * @brief Get current steal behavior.
   * @return Current StealBehavior.
   */
  [[nodiscard]] StealBehavior GetStealBehavior() const;

  /**
   * @brief Get count of all active voices.
   * @return Active voice count (real + virtual).
   */
  [[nodiscard]] uint32_t GetActiveVoiceCount() const;

  /**
   * @brief Get count of real (playing) voices.
   * @return Real voice count.
   */
  [[nodiscard]] uint32_t GetRealVoiceCount() const;

  /**
   * @brief Get count of virtual voices.
   * @return Virtual voice count.
   */
  [[nodiscard]] uint32_t GetVirtualVoiceCount() const;

  /// @}

  /// @name Mix Zone API
  /// @{

  /**
   * @brief Add a mix zone.
   * @param name Unique zone name.
   * @param snapshotName Snapshot to apply.
   * @param pos Zone center position.
   * @param inner Inner radius.
   * @param outer Outer radius.
   * @param priority Zone priority.
   * @param fadeIn Fade-in time.
   * @param fadeOut Fade-out time.
   */
  void AddMixZone(const std::string &name, const std::string &snapshotName,
                  const Vector3 &pos, float inner, float outer,
                  uint8_t priority = 128, float fadeIn = 0.5f,
                  float fadeOut = 0.5f);

  /**
   * @brief Remove a mix zone.
   * @param name Zone name.
   */
  void RemoveMixZone(const std::string &name);

  /**
   * @brief Set callback for zone entry.
   * @param cb Callback function.
   */
  void SetZoneEnterCallback(ZoneEnterCallback cb);

  /**
   * @brief Set callback for zone exit.
   * @param cb Callback function.
   */
  void SetZoneExitCallback(ZoneExitCallback cb);

  /**
   * @brief Get the currently active mix zone.
   * @return Reference to zone name (empty if none).
   */
  [[nodiscard]] const std::string &GetActiveMixZone() const;

  /// @}

  /// @name Reverb Bus API
  /// @{

  /**
   * @brief Create a reverb bus with custom parameters.
   * @param name Unique bus name.
   * @param roomSize Room size (0.0-1.0).
   * @param damp High frequency damping (0.0-1.0).
   * @param wet Wet/dry mix (0.0-1.0).
   * @param width Stereo width (0.0-1.0).
   * @return Status indicating success or error.
   */
  Status CreateReverbBus(const std::string &name, float roomSize = 0.5f,
                         float damp = 0.5f, float wet = 0.5f,
                         float width = 1.0f);

  /**
   * @brief Create a reverb bus from a preset.
   * @param name Unique bus name.
   * @param preset Reverb preset.
   * @return Status indicating success or error.
   */
  Status CreateReverbBus(const std::string &name, ReverbPreset preset);

  /**
   * @brief Get a reverb bus by name.
   * @param name Bus name.
   * @return Result containing shared_ptr to ReverbBus or error.
   */
  [[nodiscard]] Result<std::shared_ptr<ReverbBus>>
  GetReverbBus(const std::string &name);

  /**
   * @brief Set reverb parameters with optional fade.
   * @param name Reverb bus name.
   * @param wet Wet level.
   * @param roomSize Room size.
   * @param damp Damping.
   * @param fadeTime Transition time.
   */
  void SetReverbParams(const std::string &name, float wet, float roomSize,
                       float damp, float fadeTime = 0.0f);

  /**
   * @brief Add a reverb zone.
   * @param name Unique zone name.
   * @param reverbBusName Target reverb bus.
   * @param pos Zone center position.
   * @param inner Inner radius.
   * @param outer Outer radius.
   * @param priority Zone priority.
   */
  void AddReverbZone(const std::string &name, const std::string &reverbBusName,
                     const Vector3 &pos, float inner, float outer,
                     uint8_t priority = 128);

  /**
   * @brief Remove a reverb zone.
   * @param name Zone name.
   */
  void RemoveReverbZone(const std::string &name);

  /**
   * @brief Set reverb parameters in a snapshot.
   * @param snapshotName Snapshot name.
   * @param reverbBusName Reverb bus name.
   * @param wet Wet level.
   * @param roomSize Room size.
   * @param damp Damping.
   * @param width Stereo width.
   */
  void SetSnapshotReverbParams(const std::string &snapshotName,
                               const std::string &reverbBusName, float wet,
                               float roomSize, float damp, float width = 1.0f);

  /**
   * @brief Get names of all active reverb zones.
   * @return Vector of active zone names.
   */
  [[nodiscard]] std::vector<std::string> GetActiveReverbZones() const;

  /// @}

  /// @name Occlusion API
  /// @{

  /**
   * @brief Set the occlusion query callback.
   *
   * The game engine must provide this to enable occlusion.
   * @param callback Raycast callback function.
   */
  void SetOcclusionQueryCallback(OcclusionQueryCallback callback);

  /**
   * @brief Register a custom occlusion material.
   * @param mat Material to register.
   */
  void RegisterOcclusionMaterial(const OcclusionMaterial &mat);

  /**
   * @brief Enable or disable occlusion.
   * @param enabled true to enable.
   */
  void SetOcclusionEnabled(bool enabled);

  /**
   * @brief Set the occlusion threshold.
   * @param threshold Threshold value (0.0-1.0).
   */
  void SetOcclusionThreshold(float threshold);

  /**
   * @brief Set occlusion smoothing time.
   * @param seconds Interpolation time.
   */
  void SetOcclusionSmoothingTime(float seconds);

  /**
   * @brief Set occlusion update rate.
   * @param hz Updates per second.
   */
  void SetOcclusionUpdateRate(float hz);

  /**
   * @brief Set lowpass filter range for occlusion.
   * @param minFreq Minimum frequency at full occlusion.
   * @param maxFreq Maximum frequency at no occlusion.
   */
  void SetOcclusionLowPassRange(float minFreq, float maxFreq);

  /**
   * @brief Set maximum volume reduction from occlusion.
   * @param maxReduction Maximum reduction (0.0-1.0).
   */
  void SetOcclusionVolumeReduction(float maxReduction);

  /**
   * @brief Check if occlusion is enabled.
   * @return true if enabled.
   */
  [[nodiscard]] bool IsOcclusionEnabled() const;

  /// @}

  /// @name Ducking API
  /// @{

  /**
   * @brief Add a ducking rule for automatic volume control.
   *
   * When audio plays on the sidechain bus, the target bus volume
   * is automatically reduced.
   *
   * @param targetBus Bus to duck (e.g., "Music").
   * @param sidechainBus Bus that triggers ducking (e.g., "Dialogue").
   * @param duckLevel Volume when ducked (0.0-1.0, default: 0.3).
   * @param attackTime Fade down time in seconds (default: 0.1).
   * @param releaseTime Fade up time in seconds (default: 0.5).
   * @param holdTime Hold ducked level after sidechain stops (default: 0.1).
   */
  void AddDuckingRule(const std::string &targetBus,
                      const std::string &sidechainBus, float duckLevel = 0.3f,
                      float attackTime = 0.1f, float releaseTime = 0.5f,
                      float holdTime = 0.1f);

  /**
   * @brief Remove a ducking rule.
   * @param targetBus Target bus name.
   * @param sidechainBus Sidechain bus name.
   */
  void RemoveDuckingRule(const std::string &targetBus,
                         const std::string &sidechainBus);

  /**
   * @brief Check if a bus is currently being ducked.
   * @param targetBus Bus name to check.
   * @return true if the bus is being ducked.
   */
  [[nodiscard]] bool IsDucking(const std::string &targetBus) const;

  /// @}

  /// @name Doppler API
  /// @{

  /**
   * @brief Set velocity for a playing voice (for Doppler calculation).
   * @param id Voice ID.
   * @param velocity Velocity vector in world units per second.
   */
  void SetVoiceVelocity(VoiceID id, const Vector3 &velocity);

  /**
   * @brief Enable or disable Doppler effect globally.
   * @param enabled True to enable Doppler processing.
   */
  void SetDopplerEnabled(bool enabled);

  /**
   * @brief Set the speed of sound for Doppler calculations.
   * @param speed Speed of sound in world units per second (default: 343 m/s).
   */
  void SetSpeedOfSound(float speed);

  /**
   * @brief Set the Doppler exaggeration factor.
   * @param factor Multiplier for Doppler effect (1.0 = realistic, >1 =
   * exaggerated).
   */
  void SetDopplerFactor(float factor);

  /// @}

  /// @name Engine Access
  /// @{

  /**
   * @brief Get direct access to the SoLoud engine.
   * @return Reference to the SoLoud::Soloud instance.
   */
  [[nodiscard]] SoLoud::Soloud &Engine();

  /// @}

private:
  void UpdateMixZones(const Vector3 &listenerPos);
  void UpdateReverbZones(const Vector3 &listenerPos);

  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace Orpheus
