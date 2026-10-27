#ifndef COLLISION_RESPONSE_SYSTEM_HPP
#define COLLISION_RESPONSE_SYSTEM_HPP

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"

/**
 * CollisionResponseSystem - Game-level system for collision consequences
 *
 *
 * Reads per-entity CollidedWith components (written by the engine-level
 * CollisionSystem) and applies bullet-hell damage using tag + Health
 * components. Damage is resolved from the victim's side so each hit counts
 * once; collision layer/mask decides who can hit whom.
 *
 * Player bullet hits enemy/boss: subtract enemy Health, DestroyRequest the bullet;
 *   on Health <= 0 the enemy gets a DestroyRequest.
 * Enemy bullet (or enemy body) hits player: subtract player Health, DestroyRequest
 *   the bullet; on Health <= 0 set game.state = GAME_OVER.
 *
 * Reads: CollidedWith, BulletTag, EnemyTag, ShipTag, Health,
 *        Blackboard (game.state, bullet.damage, enemy.contact_damage)
 * Writes: Health (current), DestroyRequest (bullets and dead enemies),
 *         Blackboard (game.state)
 */
class CollisionResponseSystem {
public:
    void update(ComponentStorage& storage, Blackboard& blackboard);
};

#endif // COLLISION_RESPONSE_SYSTEM_HPP
