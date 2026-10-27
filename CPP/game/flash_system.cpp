#include "game/flash_system.hpp"
#include <algorithm>
#include <cmath>

void FlashSystem::update(ComponentStorage& storage, const Blackboard& blackboard) {
    const float dt = static_cast<float>(blackboard.get_or<double>("delta_time", 0.0));

    // Tunable from GameData.json's "flash" block (see the loader). The tint is
    // an RGB the entity is set to while flashing — it shows on rect entities
    // (enemies); sprite entities (player) ignore RGB and only alpha-blink, since
    // SDL texture color-mod can't brighten. dim_alpha is the low end of the blink.
    const auto clamp8 = [](int v) { return static_cast<uint8_t>(std::clamp(v, 0, 255)); };
    const uint8_t tint_r  = clamp8(blackboard.get_or<int>("flash.r", 255));
    const uint8_t tint_g  = clamp8(blackboard.get_or<int>("flash.g", 255));
    const uint8_t tint_b  = clamp8(blackboard.get_or<int>("flash.b", 255));
    const uint8_t dim_a   = clamp8(blackboard.get_or<int>("flash.dim_alpha", 60));
    const float   period  = static_cast<float>(blackboard.get_or<double>("flash.period", 0.06));

    for (Entity e : storage.entities_with_component<Flash>()) {
        auto flash_opt = storage.get_component<Flash>(e);
        auto color_opt = storage.get_component<Color>(e);
        if (!flash_opt.has_value()) continue;
        if (!color_opt.has_value()) {
            storage.remove_component<Flash>(e);   // nothing to modulate
            continue;
        }

        Flash& f = flash_opt->get();
        Color& c = color_opt->get();

        f.remaining -= dt;
        if (f.remaining <= 0.0f) {
            c = f.original;                        // restore full original Color
            storage.remove_component<Flash>(e);
        } else {
            // Tint (rects show it) + alpha strobe between original and dim.
            c.r = tint_r;
            c.g = tint_g;
            c.b = tint_b;
            const bool bright = (period > 0.0f) &&
                                std::fmod(f.remaining, period) < (period * 0.5f);
            c.a = bright ? f.original.a : dim_a;
        }
    }
}
