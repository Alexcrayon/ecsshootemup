#ifndef SPRITE_ANIM_SYSTEM_HPP
#define SPRITE_ANIM_SYSTEM_HPP

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"

/**
 * SpriteAnimSystem - cheap whole-image frame looping.
 *
 * Cycles Images::active_index for any entity whose Images has 2+ filenames, so a
 * multi-frame sprite loops. This is NOT the Class-080 atlas pipeline: it swaps
 * between separate whole images, not frames sliced from one texture — right for a
 * 2-frame ship loop, wrong for a big frame sheet.
 *
 * Opt-in by data: entities with a single filename (e.g. the background) are
 * skipped, so nothing animates unless its Images lists more than one frame.
 *
 * Reads:  Blackboard (delta_time, anim.frame_duration), Images
 * Writes: Images::active_index
 */
class SpriteAnimSystem {
public:
    void update(ComponentStorage& storage, const Blackboard& blackboard);

private:
    // Shared clock for all animated sprites (they loop in lockstep — fine for a
    // single player; add a per-entity offset later if enemies need to desync).
    float elapsed_ = 0.0f;
};

#endif // SPRITE_ANIM_SYSTEM_HPP
