#ifndef ENEMY_SPAWN_SYSTEM_HPP
#define ENEMY_SPAWN_SYSTEM_HPP

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/entity_manager.hpp"
#include "engine/ecs/blackboard.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

/**
 * EnemySpawnSystem - creates enemies from wave definitions and advances waves.
 *
 *
 * this system must run BEFORE ScriptSystem in the frame.
 * spawn() writes per-entity params to the Blackboard, and enemy.lua's on_init
 * only seeds its own defaults for keys that are absent — so if ScriptSystem ran
 * first, the script's defaults would win and archetype tuning would be ignored.
 *
 * Reads:  Blackboard (game.state, delta_time), EnemyTag count
 * Writes: new enemy entities, Blackboard (game.wave, game.all_waves_cleared,
 *         entity.<id>.<param> for each spawned enemy)
 */
class EnemySpawnSystem {
public:
    /// A recipe for one kind of enemy. Values come from "enemy_types" in JSON.
    struct Archetype {
        int hp = 1;
        float width = 30.0f;
        float height = 30.0f;
        Color color{255, 80, 80, 255};         // used when `image` is empty
        std::string image;                      // optional sprite; empty = colored rect
        uint8_t layer = 4;                      // enemy collision layer
        uint8_t mask = 3;                       // hit by player (1) + player bullets (2)
        std::string script = "scripts/enemy.lua";
        // Per-entity script tuning, published as entity.<id>.<name>.
        std::map<std::string, double> params;
    };

    // One scheduled spawn inside a wave.
    struct SpawnEntry {
        std::string type;      // key into Config::types
        float x = 0.0f;
        float y = 0.0f;
        float delay = 0.0f;    // seconds after this wave becomes active
        // Optional per-spawn param overrides: written on top of the archetype's
        // params, so this one instance can differ (e.g. a fan with burst_count 1)
        // without touching the shared archetype.
        std::map<std::string, double> params;
    };

    struct Wave {
        // Grace period after the previous wave is cleared before this wave's
        // spawns begin. A spawn fires at start_delay + SpawnEntry::delay.
        float start_delay = 0.0f;
        std::vector<SpawnEntry> spawns;
    };

    // Everything parsed out of GameData.json.
    struct Config {
        std::map<std::string, Archetype> types;
        std::vector<Wave> waves;
    };

    // Install wave data. Takes plain data (no file I/O) so tests can build a
    // Config in code — this is what keeps update() unit-testable.
    void configure(Config config);

    void update(ComponentStorage& storage,
                EntityManager& entity_manager,
                Blackboard& blackboard);

    // Back to wave 0 with a clean clock. Called on entering PLAYING.
    void reset();

    std::size_t current_wave() const { return wave_index_; }
    std::size_t wave_count() const { return config_.waves.size(); }
    bool all_waves_cleared() const { return wave_index_ >= config_.waves.size(); }

private:
    // Build one enemy entity from its archetype.
    void spawn(const SpawnEntry& entry,
               ComponentStorage& storage,
               EntityManager& entity_manager,
               Blackboard& blackboard);

    // Point state at the current wave: zero the clock and size `spawned_` to
    // the active wave (empty once all waves are cleared).
    void arm_wave();

    Config config_;

    std::size_t wave_index_ = 0;    // which wave is active
    float wave_clock_ = 0.0f;       // seconds since the active wave became active
    std::string prev_state_;        // detects the transition into PLAYING

 
    std::vector<bool> spawned_;
};

/**
 * Read "enemy_types" and "waves" out of GameData.json.
 *
*/
EnemySpawnSystem::Config load_enemy_config(const std::string& gamedata_path);

#endif // ENEMY_SPAWN_SYSTEM_HPP
