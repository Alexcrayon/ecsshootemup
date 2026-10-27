#include "game/bullet_spawn_system.hpp"
#include "engine/ecs/bullet_factory.hpp"
#include <cmath>
#include <algorithm>

void BulletSpawnSystem::update(ComponentStorage& storage, Blackboard& blackboard, EntityManager& entity_manager) {
    double delta_time = blackboard.get<double>("delta_time");
    float dt = static_cast<float>(delta_time);

    // Read config from Blackboard with defaults
    float bullet_speed = blackboard.get_or<float>("bullet.speed", 400.0f);
    float bullet_lifetime = blackboard.get_or<float>("bullet.lifetime", 2.0f);
    float bullet_size = blackboard.get_or<float>("bullet.size", 4.0f);
    int max_live = blackboard.get_or<int>("bullet.max_live", 6);
    float fire_cooldown_config = blackboard.get_or<float>("bullet.fire_cooldown", 0.25f);
    int bullet_layer = blackboard.get_or<int>("bullet.layer", 2);
    int bullet_mask = blackboard.get_or<int>("bullet.mask", 4);

    // Decrement cooldown, clamp at zero
    float cooldown = blackboard.get_or<float>("ship.fire_cooldown_remaining", 0.0f);
    cooldown = std::max(0.0f, cooldown - dt);
    blackboard.set("ship.fire_cooldown_remaining", cooldown);

    // Iterate entities with Input
    auto entities = storage.entities_with_component<Input>();
    for (Entity entity : entities) {
        auto& input = storage.get_component<Input>(entity)->get();
        if (!input.fire) continue;

        // Need Position to spawn
        if (!storage.has_component<Position>(entity)) continue;
    

        // Check cooldown
        if (cooldown > 0.0f) continue;

        // Check max live cap
        //auto bullet_count = storage.entities_with_component<BulletTag>().size();

        //only count the player bullet to aviod maxout bullet through enemy
        int player_bullet_layer = blackboard.get_or<int>("bullet.layer", 2);
        int live = 0;
        for (Entity b : storage.entities_with_component<BulletTag>()) {
            auto col = storage.get_component<Collider>(b);
            if (col.has_value() && col->get().layer == static_cast<uint8_t>(player_bullet_layer)) ++live;
        }
        if (live >= max_live) continue;
        //if (static_cast<int>(bullet_count) >= max_live) continue;

        // Get ship state
        auto& pos = storage.get_component<Position>(entity)->get();
        // NEW (Class-110): resolve the firing angle without requiring Rotation.
        // 0 radians = straight up, which is what a vertical shmup wants.
        float fire_angle = 0.0f;
        if (storage.has_component<Rotation>(entity)) {
            fire_angle = storage.get_component<Rotation>(entity)->get().angle;
        }

        //remove capture current ship velocity
        /*float ship_dx = 0.0f, ship_dy = 0.0f;
        if (storage.has_component<Velocity>(entity)) {
            auto& vel = storage.get_component<Velocity>(entity)->get();
            ship_dx = vel.dx;
            ship_dy = vel.dy;
        }*/

        // Compute bullet spawn at the ship's nose (tip)
        // Ship position is bottom-left corner; center is (pos.x + half, pos.y + half)
        // Facing direction: (-sin(angle), cos(angle)) — texture points up
        float half_ship = 0.0f;
        if (storage.has_component<Size>(entity)) {
            auto& sz = storage.get_component<Size>(entity)->get();
            half_ship = sz.width / 2.0f;
        }
        float center_x = pos.x + half_ship;
        float center_y = pos.y + half_ship;
        float dir_x = -std::sin(fire_angle);
        float dir_y = std::cos(fire_angle);
        // Bullet CENTRE at the ship's nose; the factory converts to the Position
        // corner. dir points along the firing angle (0 = straight up).
        float nose_cx = center_x + dir_x * half_ship;
        float nose_cy = center_y + dir_y * half_ship;

        // Build through the shared factory so the player and the enemy scripts
        // (engine.fire_bullet) construct the identical bullet entity. The recipe
        // (Velocity/Lifetime/IgnoreWorldBounds/Collider/BulletTag) lives in
        // engine/ecs/bullet_factory.hpp, not copied here and in the Lua scripts.
        BulletSpec spec;
        spec.x        = nose_cx;
        spec.y        = nose_cy;
        spec.vx       = bullet_speed * dir_x;
        spec.vy       = bullet_speed * dir_y;
        spec.size     = bullet_size;
        spec.layer    = static_cast<uint8_t>(bullet_layer);
        spec.mask     = static_cast<uint8_t>(bullet_mask);
        spec.lifetime = bullet_lifetime;
        spec.sprite   = "double_bullet.png";   // Color stays white as a fallback
        spawn_bullet(storage, entity_manager, spec);

        // Reset cooldown
        cooldown = fire_cooldown_config;
        blackboard.set("ship.fire_cooldown_remaining", cooldown);
    }
}
