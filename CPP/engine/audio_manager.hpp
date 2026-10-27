#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <cstddef>

class AudioManager {
public:
    AudioManager(MIX_Mixer* mixer, const std::string& assets_dir);
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    // Resolves path: assets_dir/audio/<name>.
    // Cache hit  -> returns cached pointer.
    // Cache miss -> calls MIX_LoadAudio; on success caches and returns; on
    //               failure logs to stderr and returns nullptr (no negative
    //               caching, so a later retry can still succeed).
    // nullptr mixer -> returns nullptr silently (headless path).
    MIX_Audio* get_audio(const std::string& name);

    // Number of successfully cached audio handles. For test introspection.
    size_t audio_count() const;

private:
    MIX_Mixer* mixer_;
    std::string assets_directory_;
    std::unordered_map<std::string, MIX_Audio*> cache_;
};

#endif // AUDIO_MANAGER_HPP
