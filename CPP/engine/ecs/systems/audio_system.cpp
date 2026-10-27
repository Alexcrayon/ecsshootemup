#include "engine/ecs/systems/audio_system.hpp"

#include "engine/ecs/components.hpp"
#include "engine/audio_manager.hpp"

#include <cstdio>

AudioSystem::AudioSystem(MIX_Mixer* mixer, AudioManager& audio_manager)
    : mixer_(mixer), audio_manager_(audio_manager) {
    if (!mixer_) return;

    tracks_.reserve(AUDIO_TRACK_POOL_SIZE);
    for (size_t i = 0; i < AUDIO_TRACK_POOL_SIZE; ++i) {
        MIX_Track* t = MIX_CreateTrack(mixer_);
        if (t) tracks_.push_back(t);
    }
}

AudioSystem::~AudioSystem() {
    for (MIX_Track* t : tracks_) {
        if (t) MIX_DestroyTrack(t);
    }
    tracks_.clear();
}

void AudioSystem::update(ComponentStorage& components) {
    if (!mixer_) return;

    auto entities = components.entities_with_component<AudioSource>();
    for (Entity entity : entities) {
        if (played_set_.count(entity)) continue;

        auto source_opt = components.get_component<AudioSource>(entity);
        if (!source_opt.has_value()) continue;
        const AudioSource& source = source_opt->get();

        played_set_.insert(entity);
        if (!source.play_on_add) continue;

        play_sound(source.filename, source.volume, source.looping);
    }
}

bool AudioSystem::play_sound(const std::string& filename, float volume, bool looping) {
    if (!mixer_) return false;

    MIX_Audio* audio = audio_manager_.get_audio(filename);
    if (!audio) return false;

    MIX_Track* track = find_free_track();
    if (!track) {
        fprintf(stderr, "[AudioSystem] Track pool exhausted; dropping sound: %s\n",
                filename.c_str());
        return false;
    }

    MIX_SetTrackAudio(track, audio);
    MIX_SetTrackGain(track, volume);
    MIX_PlayTrack(track, looping ? -1 : 0);
    return true;
}

MIX_Track* AudioSystem::find_free_track() {
    for (MIX_Track* t : tracks_) {
        if (t && !MIX_TrackPlaying(t)) return t;
    }
    return nullptr;
}

size_t AudioSystem::played_count() const {
    return played_set_.size();
}

size_t AudioSystem::track_pool_size() const {
    return tracks_.size();
}
