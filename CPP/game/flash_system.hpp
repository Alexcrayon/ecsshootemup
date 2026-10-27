#ifndef FLASH_SYSTEM_HPP
#define FLASH_SYSTEM_HPP

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"

/**
 * FlashSystem - drives the brief hit flash.
 *
 * For each entity with a Flash component, strobes its Color alpha for the flash
 * duration, then restores Flash::orig_alpha and removes the component. Relies on
 * RenderSystem honoring Color.a for both rects (blend) and sprites (alpha-mod).
 *
 * Runs every frame (not gated on PLAYING) so a flash still finishes and restores
 * if the game leaves the play state mid-flash.
 *
 * Reads:  Blackboard (delta_time), Flash, Color
 * Writes: Color::a, removes Flash when it expires
 */
class FlashSystem {
public:
    void update(ComponentStorage& storage, const Blackboard& blackboard);
};

#endif // FLASH_SYSTEM_HPP
