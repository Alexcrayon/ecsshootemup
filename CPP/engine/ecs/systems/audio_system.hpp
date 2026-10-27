/**
 * AudioSystem - System for playing sounds declared via AudioSource components.
 *
 * Each frame, scans entities with AudioSource. For any entity not yet seen,
 * inserts it into played_set; if play_on_add is true, looks up the audio via
 * AudioManager, picks a free track from a fixed-size pool, and plays it once.
 * Subsequent frames skip already-seen entities, so play_on_add fires exactly
 * once per entity.
 *
 * Track pool is fixed at AUDIO_TRACK_POOL_SIZE (16). If all tracks are busy
 * when a new sound triggers, the sound is silently dropped — no crash, no
 * assert. By design.
 *
 * Headless / no-audio case: when constructed with a nullptr mixer, the
 * system holds an empty track pool and update() is a no-op. No entries are
 * added to played_set, so the system imposes no work in headless CI.
 */

#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <SDL3_mixer/SDL_mixer.h>
#include <vector>
#include <unordered_set>
#include <cstddef>
#include <string>

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/entity_manager.hpp"

class AudioManager;

class AudioSystem {
public:
    static constexpr size_t AUDIO_TRACK_POOL_SIZE = 16;

    AudioSystem(MIX_Mixer* mixer, AudioManager& audio_manager);
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    void update(ComponentStorage& components);

    // Plays a sound immediately, on demand. Loads (or reuses cached) audio via
    // AudioManager, claims a free track from the pool, sets gain, and plays.
    // Returns false — without crashing — when the mixer is null, the audio
    // fails to load, or the track pool is exhausted. This is the same playback
    // path update() uses for play_on_add; it is also the entry point exposed to
    // Lua as engine.play_sound.
    bool play_sound(const std::string& filename, float volume = 1.0f, bool looping = false);

    // For test introspection.
    size_t played_count() const;
    size_t track_pool_size() const;

private:
    MIX_Mixer* mixer_;
    AudioManager& audio_manager_;
    std::vector<MIX_Track*> tracks_;
    std::unordered_set<Entity> played_set_;

    // Returns first track with MIX_TrackPlaying == false, or nullptr if pool exhausted.
    MIX_Track* find_free_track();
};

#endif // AUDIO_SYSTEM_HPP
