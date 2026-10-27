/**
 * Unit tests for EnemySpawnSystem (game/enemy_spawn_system.hpp)
 *
 * Drives update() with a Config built in code (no file I/O — that's why
 * load_enemy_config is a separate free function). Covers:
 *   - only spawns while game.state == PLAYING
 *   - a spawn fires once, at start_delay + entry.delay into the wave
 *   - the clear-gate: a wave advances only when all spawns fired AND no EnemyTag
 *     entity is left alive
 *   - game.all_waves_cleared / game.wave bookkeeping
 *   - retry: re-entering PLAYING resets to wave 0 and DestroyRequests leftovers
 *   - spawn() attaches the archetype's components and publishes its params
 *   - an unknown enemy type is skipped, not fatal
 */

#include <catch2/catch_test_macros.hpp>
#include "game/enemy_spawn_system.hpp"
#include "game/game_state_system.hpp"
#include "engine/ecs/entity_manager.hpp"

namespace {

using Archetype  = EnemySpawnSystem::Archetype;
using SpawnEntry = EnemySpawnSystem::SpawnEntry;
using Wave       = EnemySpawnSystem::Wave;
using Config     = EnemySpawnSystem::Config;

Archetype make_grunt(int hp = 2) {
    Archetype a;
    a.hp = hp;
    a.width = 20.0f;
    a.height = 20.0f;
    return a;
}

struct World {
    ComponentStorage storage;
    EntityManager em;
    Blackboard bb;
    EnemySpawnSystem sys;

    World() {
        bb.set<double>("delta_time", 1.0 / 60.0);
        bb.set<std::string>("game.state", std::string(game_state::TITLE));
    }

    void set_state(const char* s) { bb.set<std::string>("game.state", std::string(s)); }
    void set_dt(double dt)        { bb.set<double>("delta_time", dt); }
    void step()                   { sys.update(storage, em, bb); }

    std::size_t enemies() const {
        return storage.entities_with_component<EnemyTag>().size();
    }
    // Simulate the destruction pipeline just enough for the clear-gate: drop the
    // EnemyTag so entities_with_component<EnemyTag>() reports the arena as clear.
    void kill_all_enemies() {
        for (Entity e : storage.entities_with_component<EnemyTag>()) {
            storage.remove_component<EnemyTag>(e);
        }
    }
};

// One wave, one grunt, both delays zero unless overridden.
Config one_wave(float start_delay = 0.0f, float spawn_delay = 0.0f) {
    Config c;
    c.types["grunt"] = make_grunt();
    Wave w;
    w.start_delay = start_delay;
    w.spawns.push_back(SpawnEntry{"grunt", 0.0f, 0.0f, spawn_delay});
    c.waves.push_back(std::move(w));
    return c;
}

}  // namespace

TEST_CASE("Nothing spawns outside PLAYING", "[enemy_spawn][unit]") {
    World w;
    w.sys.configure(one_wave());

    for (int i = 0; i < 5; ++i) w.step();   // still at TITLE
    REQUIRE(w.enemies() == 0);
    REQUIRE(w.sys.current_wave() == 0);
}

TEST_CASE("Entering PLAYING spawns the wave's due enemies", "[enemy_spawn][unit]") {
    World w;
    w.sys.configure(one_wave());
    w.set_state(game_state::PLAYING);

    w.step();                                // start_delay 0, delay 0 -> due now
    REQUIRE(w.enemies() == 1);
    REQUIRE(w.bb.get_or<int>("game.wave", -1) == 1);
}

TEST_CASE("A spawn fires only after start_delay + delay", "[enemy_spawn][unit]") {
    World w;
    w.sys.configure(one_wave(/*start_delay=*/0.5f, /*spawn_delay=*/0.0f));
    w.set_state(game_state::PLAYING);
    w.set_dt(0.2);

    w.step();  REQUIRE(w.enemies() == 0);    // clock 0.2 < 0.5
    w.step();  REQUIRE(w.enemies() == 0);    // clock 0.4 < 0.5
    w.step();  REQUIRE(w.enemies() == 1);    // clock 0.6 >= 0.5
}

TEST_CASE("A spawn fires exactly once", "[enemy_spawn][unit]") {
    World w;
    w.sys.configure(one_wave());
    w.set_state(game_state::PLAYING);

    for (int i = 0; i < 10; ++i) w.step();
    REQUIRE(w.enemies() == 1);               // the one grunt, never duplicated
}

TEST_CASE("A wave does not advance while an enemy is alive", "[enemy_spawn][unit]") {
    World w;
    Config c = one_wave();
    c.waves.push_back(one_wave().waves[0]);  // a second, identical wave
    w.sys.configure(c);
    w.set_state(game_state::PLAYING);

    w.step();
    REQUIRE(w.sys.current_wave() == 0);
    for (int i = 0; i < 10; ++i) w.step();   // enemy still alive
    REQUIRE(w.sys.current_wave() == 0);
}

TEST_CASE("A wave advances once cleared, next wave spawns after", "[enemy_spawn][unit]") {
    World w;
    Config c = one_wave();
    c.waves.push_back(one_wave().waves[0]);
    w.sys.configure(c);
    w.set_state(game_state::PLAYING);

    w.step();                                // wave 0 grunt spawns
    REQUIRE(w.sys.current_wave() == 0);
    REQUIRE(w.enemies() == 1);

    w.kill_all_enemies();
    w.step();                                // all emitted + clear -> advance
    REQUIRE(w.sys.current_wave() == 1);
    REQUIRE(w.enemies() == 0);               // wave 1 not spawned yet this frame

    w.step();                                // now wave 1 spawns
    REQUIRE(w.enemies() == 1);
    REQUIRE(w.bb.get_or<int>("game.wave", -1) == 2);
}

TEST_CASE("Clearing the last wave sets game.all_waves_cleared", "[enemy_spawn][unit]") {
    World w;
    w.sys.configure(one_wave());
    w.set_state(game_state::PLAYING);

    w.step();
    w.kill_all_enemies();
    w.step();

    REQUIRE(w.sys.all_waves_cleared());
    REQUIRE(w.bb.get_or<bool>("game.all_waves_cleared", false) == true);
}

TEST_CASE("Re-entering PLAYING resets to wave 0 and clears leftovers",
          "[enemy_spawn][unit]") {
    World w;
    Config c = one_wave();
    c.waves.push_back(one_wave().waves[0]);
    w.sys.configure(c);

    // Reach wave 1 with a live enemy.
    w.set_state(game_state::PLAYING);
    w.step();
    w.kill_all_enemies();
    w.step();
    w.step();                                // wave 1 grunt now alive
    REQUIRE(w.sys.current_wave() == 1);
    REQUIRE(w.enemies() == 1);
    Entity leftover = w.storage.entities_with_component<EnemyTag>()[0];

    // Lose, then retry.
    w.set_state(game_state::GAME_OVER);
    w.step();
    REQUIRE(w.sys.current_wave() == 1);      // frozen while not PLAYING

    w.set_state(game_state::PLAYING);
    w.step();
    REQUIRE(w.sys.current_wave() == 0);                          // reset
    REQUIRE(w.storage.has_component<DestroyRequest>(leftover));  // old enemy marked
}

TEST_CASE("spawn attaches archetype components and publishes params",
          "[enemy_spawn][unit]") {
    World w;
    Config c;
    Archetype a = make_grunt(/*hp=*/4);
    a.width = 25.0f;
    a.height = 25.0f;
    a.params["fire_interval"] = 0.5;
    a.params["hold_y"]        = 200.0;
    c.types["grunt"] = a;
    Wave wave;
    wave.spawns.push_back(SpawnEntry{"grunt", 10.0f, 20.0f, 0.0f});
    c.waves.push_back(std::move(wave));

    w.sys.configure(c);
    w.set_state(game_state::PLAYING);
    w.step();

    auto ids = w.storage.entities_with_component<EnemyTag>();
    REQUIRE(ids.size() == 1);
    Entity e = ids[0];

    REQUIRE(w.storage.get_component<Position>(e)->get().x == 10.0f);
    REQUIRE(w.storage.get_component<Position>(e)->get().y == 20.0f);
    REQUIRE(w.storage.get_component<Size>(e)->get().width == 25.0f);
    REQUIRE(w.storage.get_component<Health>(e)->get().current == 4);
    REQUIRE(w.storage.get_component<Health>(e)->get().max == 4);
    REQUIRE(w.storage.has_component<Script>(e));
    // enemy.lua attaches IgnoreWorldBounds itself; the spawner must NOT.
    REQUIRE_FALSE(w.storage.has_component<IgnoreWorldBounds>(e));

    const std::string prefix = "entity." + std::to_string(e) + ".";
    REQUIRE(w.bb.get_or<double>(prefix + "fire_interval", -1.0) == 0.5);
    REQUIRE(w.bb.get_or<double>(prefix + "hold_y", -1.0) == 200.0);
}

TEST_CASE("Unknown enemy type is skipped, not fatal", "[enemy_spawn][unit]") {
    World w;
    Config c;
    c.types["grunt"] = make_grunt();
    Wave wave;
    wave.spawns.push_back(SpawnEntry{"ghost", 0.0f, 0.0f, 0.0f});  // no such type
    c.waves.push_back(std::move(wave));

    w.sys.configure(c);
    w.set_state(game_state::PLAYING);
    w.step();

    REQUIRE(w.enemies() == 0);               // nothing spawned, no crash
}
