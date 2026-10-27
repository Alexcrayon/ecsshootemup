#include "game/collision_response_system.hpp"
#include <string>

namespace {
// Attach a brief hit flash to a victim (FlashSystem drives + removes it).
// Guarded so rapid hits don't re-capture a mid-strobe Color as the "original".
void trigger_flash(ComponentStorage& storage, Entity e, float duration) {
    if (storage.has_component<Flash>(e)) return;   // already flashing
    auto col = storage.get_component<Color>(e);
    if (!col.has_value()) return;                  // nothing to modulate
    storage.add_component(e, Flash{duration, col->get()});  // remember original Color
}
}  // namespace

void CollisionResponseSystem::update(ComponentStorage& storage, Blackboard& blackboard) {

    // Collision layer/mask:
    //   - player bullets can only hit enemies / the boss
    //   - enemy bullets can only hit the player

    // Only respond during active play (a title / win / lose state freezes it).
    std::string state = blackboard.get_or<std::string>("game.state", std::string("PLAYING"));
    if (state != "PLAYING") {
        return;
    }

    // Data-driven damage amounts (tunable in GameData.json without recompiling).
    int bullet_damage  = blackboard.get_or<int>("bullet.damage", 1);
    int contact_damage = blackboard.get_or<int>("enemy.contact_damage", 1);
    float flash_dur    = static_cast<float>(blackboard.get_or<double>("flash.duration", 0.12));

    // Iterate every entity that recorded a collision this frame.
    auto collided_entities = storage.entities_with_component<CollidedWith>();
    for (Entity entity : collided_entities) {
        auto& collided_with = storage.get_component<CollidedWith>(entity)->get();

        // --- Enemy (or boss) takes damage from a player bullet ---
        if (storage.has_component<EnemyTag>(entity) &&
            storage.has_component<Health>(entity)) {
            auto& hp = storage.get_component<Health>(entity)->get();
            for (Entity other : collided_with.entities) {
                if (storage.has_component<BulletTag>(other)) {
                    hp.current -= bullet_damage;
                    storage.add_component(other, DestroyRequest{});  // spend the bullet
                    trigger_flash(storage, entity, flash_dur);                  // flash the enemy
                    // NEW (Class-110): request the metal hit SFX. main plays it once
                    // per frame (see the sfx.enemy_hit check after this system) so
                    // AudioSystem never leaks into game logic here.
                    blackboard.set<bool>("sfx.enemy_hit", true);
                }
            }
            if (hp.current <= 0) {
                hp.current = 0;
                // Enemy dies. Wave-clear is detected elsewhere by counting the
                // remaining EnemyTag entities.
                storage.add_component(entity, DestroyRequest{});
                // NOTE: boss win-detection belongs in the (not-yet-built)
                // GameStateSystem. Once you add a BossTag you can, for example,
                // set blackboard "game.state" = "WIN" here on the boss's death.
            }
        }

        // --- Player takes damage from an enemy bullet (or an enemy body) ---
        if (storage.has_component<ShipTag>(entity) &&
            storage.has_component<Health>(entity)) {
            auto& hp = storage.get_component<Health>(entity)->get();
            for (Entity other : collided_with.entities) {
                bool hit_by_bullet = storage.has_component<BulletTag>(other);
                bool touched_enemy = storage.has_component<EnemyTag>(other);
                if (hit_by_bullet) {
                    hp.current -= bullet_damage;
                    storage.add_component(other, DestroyRequest{});  // spend enemy bullet
                    trigger_flash(storage, entity, flash_dur);                  // flash the player
                } else if (touched_enemy) {
                    // OPTIONAL: body contact with an enemy also hurts. Delete
                    // this branch for a pure dodge-the-bullets game.
                    hp.current -= contact_damage;
                    trigger_flash(storage, entity, flash_dur);
                }
            }
            if (hp.current <= 0) {
                hp.current = 0;
                //blackboard.set<std::string>("game.state", "GAME_OVER");
                //remove game over state from here
          
            }
        }
    }
}
