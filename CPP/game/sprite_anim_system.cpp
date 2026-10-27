#include "game/sprite_anim_system.hpp"

void SpriteAnimSystem::update(ComponentStorage& storage, const Blackboard& blackboard) {
    elapsed_ += static_cast<float>(blackboard.get_or<double>("delta_time", 0.0));

    const float frame_dur =
        static_cast<float>(blackboard.get_or<double>("anim.frame_duration", 0.2));
    if (frame_dur <= 0.0f) return;   // guard: avoids divide-by-zero below

    for (Entity e : storage.entities_with_component<Images>()) {
        auto& img = storage.get_component<Images>(e)->get();
        const std::size_t n = img.filenames.size();
        if (n < 2) continue;         // single-frame sprites (e.g. background) never animate
        img.active_index = static_cast<std::size_t>(elapsed_ / frame_dur) % n;
    }
}
