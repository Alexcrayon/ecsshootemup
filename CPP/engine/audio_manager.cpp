#include "audio_manager.hpp"

#include <SDL3/SDL.h>
#include <cstdio>

AudioManager::AudioManager(MIX_Mixer* mixer, const std::string& assets_dir)
    : mixer_(mixer), assets_directory_(assets_dir) {}

AudioManager::~AudioManager() {
    for (auto& [name, audio] : cache_) {
        if (audio) MIX_DestroyAudio(audio);
    }
    cache_.clear();
}

MIX_Audio* AudioManager::get_audio(const std::string& name) {
    if (!mixer_) return nullptr;

    auto it = cache_.find(name);
    if (it != cache_.end()) return it->second;

    const std::string path = assets_directory_ + "/audio/" + name;
    MIX_Audio* audio = MIX_LoadAudio(mixer_, path.c_str(), true);
    if (!audio) {
        fprintf(stderr, "[AudioManager] Failed to load audio: %s (%s)\n",
                name.c_str(), SDL_GetError());
        return nullptr;
    }
    cache_[name] = audio;
    return audio;
}

size_t AudioManager::audio_count() const {
    return cache_.size();
}
