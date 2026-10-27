#ifndef ENGINE_ECS_BULLET_FACTORY_HPP
#define ENGINE_ECS_BULLET_FACTORY_HPP

#include <cstdint>
#include <string>

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/entity_manager.hpp"

/**
 * BulletSpec / spawn_bullet — the single definition of "what a bullet entity is".
 *
 * Both the player's BulletSpawnSystem (C++) and the enemy/boss scripts (through
 * the engine.fire_bullet Lua binding) build their projectiles here, so the
 * component recipe lives in one place instead of being copied across
 * bullet_spawn_system.cpp, enemy.lua and boss.lua.
 *
 * Callers pass the bullet's CENTRE (x, y); Position is stored as the bottom-left
 * corner, so spawn_bullet does the corner conversion from `size`.
 *
 * Header-only inline so every target that already compiles a caller (the game
 * exe via BulletSpawnSystem, the test exes via lua_bindings.cpp) gets the
 * definition without a new .cpp in the CMake source lists.
 */
struct BulletSpec {
    float x = 0.0f;          // bullet CENTRE in world coords
    float y = 0.0f;
    float vx = 0.0f;         // velocity, pixels/second
    float vy = 0.0f;
    float size = 8.0f;       // square: used for render Size and Collider
    std::uint8_t layer = 0;  // collision layer (2 = player bullet, 8 = enemy bullet)
    std::uint8_t mask = 0;   // collision mask
    float lifetime = 4.0f;   // seconds before Lifetime expires it
    Color color{255, 255, 255, 255};  // rect colour, and fallback behind a sprite
    std::string sprite;      // Images filename; empty = colored rect
    bool has_rotation = false;  // true = point the sprite along `angle`
    float angle = 0.0f;      // radians; only applied when has_rotation
};

/// Build a bullet entity from `spec` and return it. Attaches the shared bullet
/// component set: Position/Size/Velocity/Lifetime/IgnoreWorldBounds/Collider/
/// Color, an optional Images + Rotation, and BulletTag (so CollisionResponse
/// applies damage).
inline Entity spawn_bullet(ComponentStorage& storage,
                           EntityManager& entity_manager,
                           const BulletSpec& spec) {
    Entity b = entity_manager.create_entity();

    // Caller passes the centre; Position is the bottom-left corner.
    storage.add_component(b, Position{spec.x - spec.size / 2.0f,
                                      spec.y - spec.size / 2.0f});
    storage.add_component(b, Size{spec.size, spec.size});
    storage.add_component(b, Velocity{spec.vx, spec.vy});
    storage.add_component(b, Lifetime{spec.lifetime});
    // Bullets leave the arena and expire by Lifetime, so opt out of the
    // world-bound clamp in MovementSystem.
    storage.add_component(b, IgnoreWorldBounds{});
    storage.add_component(b, Collider{spec.size, spec.size, spec.layer, spec.mask});
    storage.add_component(b, spec.color);
    if (!spec.sprite.empty()) {
        // RenderSystem prefers Images over Color; Color remains as a fallback.
        storage.add_component(b, Images{{spec.sprite}, 0});
    }
    if (spec.has_rotation) {
        // Point an elongated sprite along its travel direction.
        storage.add_component(b, Rotation{spec.angle, 0.0f});
    }
    storage.add_component(b, BulletTag{});
    return b;
}

#endif  // ENGINE_ECS_BULLET_FACTORY_HPP
