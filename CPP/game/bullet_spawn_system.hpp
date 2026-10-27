#ifndef BULLET_SPAWN_SYSTEM_HPP
#define BULLET_SPAWN_SYSTEM_HPP

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"
#include "engine/ecs/entity_manager.hpp"

/**
 * BulletSpawnSystem - Game-level system for bullet firing
 *
 * Reads Input fire state, checks cooldown and live bullet cap,
 * and spawns bullet entities. Separate from ShipControlSystem
 * (no god systems).
 *
 * Reads: Input.fire, Position, Rotation.angle, Velocity (optional)
 * Writes: Creates new bullet entities, Blackboard ship.fire_cooldown_remaining
 */
class BulletSpawnSystem {
public:
    void update(ComponentStorage& storage, Blackboard& blackboard, EntityManager& entity_manager);
};

#endif // BULLET_SPAWN_SYSTEM_HPP
