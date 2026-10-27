#include "game/enemy_spawn_system.hpp"
#include "game/game_state_system.hpp"

#include <algorithm>  
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>

#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Configuration / lifecycle
// ---------------------------------------------------------------------------

void EnemySpawnSystem::configure(Config config) {
    config_ = std::move(config);

    reset();
}

void EnemySpawnSystem::reset() {
    wave_index_ = 0;
    arm_wave();
}

void EnemySpawnSystem::arm_wave() {
    wave_clock_ = 0.0f;
    if (wave_index_ < config_.waves.size()) {
        // All flags start false: nothing in the new wave has spawned yet.
        spawned_.assign(config_.waves[wave_index_].spawns.size(), false);
    } else {
        spawned_.clear();   // past the last wave — nothing left to spawn
    }
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void EnemySpawnSystem::update(ComponentStorage& storage,
                              EntityManager& entity_manager,
                              Blackboard& blackboard) {
    const std::string state = GameStateSystem::current(blackboard);

    // On entering PLAYING, start over. Also clear any enemies left from a
    // previous run
    if (state == game_state::PLAYING && prev_state_ != game_state::PLAYING) {
        reset();
        for (Entity e : storage.entities_with_component<EnemyTag>()) {
            storage.add_component(e, DestroyRequest{});
        }
        blackboard.set<int>("game.wave", 1);
        blackboard.set<bool>("game.all_waves_cleared", false);
    }
    prev_state_ = state;

    if (state != game_state::PLAYING) return;
    if (all_waves_cleared()) return;

    wave_clock_ += static_cast<float>(blackboard.get_or<double>("delta_time", 0.0));

    const Wave& wave = config_.waves[wave_index_];

    // Emit every spawn that is now due and has not fired yet. A spawn is due at
    // (wave.start_delay + entry.delay) seconds into the wave; start_delay is the
    // gap after the previous wave cleared.
    for (std::size_t i = 0; i < wave.spawns.size(); ++i) {
        const float due_at = wave.start_delay + wave.spawns[i].delay;
        if (!spawned_[i] && wave_clock_ >= due_at) {
            spawn(wave.spawns[i], storage, entity_manager, blackboard);
            spawned_[i] = true;
        }
    }

    // Advance to the next wave only when every spawn has fired AND no enemy is
    // left alive. 
  
    // destroy_marked_entities runs at the end of the frame, so "arena clear" is
    // seen the frame after the last enemy dies.
    const bool all_emitted =
        std::all_of(spawned_.begin(), spawned_.end(), [](bool fired) { return fired; });
    const bool arena_clear =
        storage.entities_with_component<EnemyTag>().empty();

    if (all_emitted && arena_clear) {
        ++wave_index_;
        arm_wave();
        if (all_waves_cleared()) {
            blackboard.set<bool>("game.all_waves_cleared", true);  // GameStateSystem -> WIN
        } else {
            blackboard.set<int>("game.wave", static_cast<int>(wave_index_) + 1);
        }
    }
}

// ---------------------------------------------------------------------------
// Spawning one enemy
// ---------------------------------------------------------------------------

void EnemySpawnSystem::spawn(const SpawnEntry& entry,
                             ComponentStorage& storage,
                             EntityManager& entity_manager,
                             Blackboard& blackboard) {
    auto it = config_.types.find(entry.type);
    if (it == config_.types.end()) {
      
        std::cerr << "[EnemySpawnSystem] unknown enemy type '" << entry.type
                  << "' — skipping spawn\n";
        return;
    }
    const Archetype& a = it->second;

    Entity id = entity_manager.create_entity();

    storage.add_component(id, Position{entry.x, entry.y});
    storage.add_component(id, Size{a.width, a.height});
    storage.add_component(id, a.color);
    if (!a.image.empty()) {
        // RenderSystem draws Images in preference to Color, so a sprite overrides
        // the rect, leave it off to render as a colored square.
        storage.add_component(id, Images{{a.image}, 0});
    }
    storage.add_component(id, Collider{a.width, a.height, a.layer, a.mask});
    storage.add_component(id, Health{a.hp, a.hp});
    storage.add_component(id, EnemyTag{});

    // Publish per-entity params BEFORE attaching Script
    const std::string prefix = "entity." + std::to_string(id) + ".";
    
    // clear params 
    for (const std::string& k : blackboard.get_all_keys()) {
        if (k.rfind(prefix, 0) == 0) {
            blackboard.remove(k);
        }
    }
    for (const auto& [name, value] : a.params) {
        blackboard.set<double>(prefix + name, value);
    }
    // Per-spawn overrides win over the archetype (written last). Lets one entry
    // tweak a shared type — e.g. a fan with burst_count 1 — without a new type.
    for (const auto& [name, value] : entry.params) {
        blackboard.set<double>(prefix + name, value);
    }

    storage.add_component(id, Script{a.script, false});
}

// ---------------------------------------------------------------------------
// JSON loading (file I/O kept out of the system so update() stays testable)
// ---------------------------------------------------------------------------

EnemySpawnSystem::Config load_enemy_config(const std::string& gamedata_path) {
    using json = nlohmann::json;
    EnemySpawnSystem::Config config;

    std::ifstream file(gamedata_path);
    if (!file.is_open()) {
        std::cerr << "[EnemySpawnSystem] could not open " << gamedata_path << "\n";
        return config;   // empty config: the game still starts, just enemy-less
    }

    json data;
    try {
        data = json::parse(file);
    } catch (const std::exception& e) {
        std::cerr << "[EnemySpawnSystem] JSON parse error: " << e.what() << "\n";
        return config;
    }

    // --- enemy_types (archetypes) ---
    if (data.contains("enemy_types")) {
        for (auto& [name, t] : data["enemy_types"].items()) {
            EnemySpawnSystem::Archetype a;
            a.hp     = t.value("hp", 1);
            a.width   = t.value("width", 30.0f);
            a.height = t.value("height", 30.0f);
            a.layer  = static_cast<uint8_t>(t.value("layer", 4));
            a.mask   = static_cast<uint8_t>(t.value("mask", 3));
            a.script = t.value("script", std::string("scripts/enemy.lua"));
            a.image  = t.value("image", std::string(""));

            if (t.contains("color")) {
                const auto& c = t["color"];
                a.color = Color{
                    static_cast<uint8_t>(c.value("r", 255)),
                    static_cast<uint8_t>(c.value("g", 80)),
                    static_cast<uint8_t>(c.value("b", 80)),
                    static_cast<uint8_t>(c.value("a", 255))
                };
            }

            if (t.contains("params")) {
                for (auto& [key, v] : t["params"].items()) {
                    if (v.is_number()) {
                        a.params[key] = v.get<double>();
                    }
                }
            }

            config.types[name] = std::move(a);
        }
    }

    // --- waves ---
    if (data.contains("waves")) {
        for (const auto& wj : data["waves"]) {
            EnemySpawnSystem::Wave w;
            w.start_delay = wj.value("start_delay", 0.0f);
            if (wj.contains("spawns")) {
                for (const auto& sj : wj["spawns"]) {
                    EnemySpawnSystem::SpawnEntry e;
                    e.type  = sj.value("type", std::string(""));
                    e.x     = sj.value("x", 0.0f);
                    e.y     = sj.value("y", 0.0f);
                    e.delay = sj.value("delay", 0.0f);
                    if (sj.contains("params")) {   // optional per-spawn overrides
                        for (auto& [key, v] : sj["params"].items()) {
                            if (v.is_number()) e.params[key] = v.get<double>();
                        }
                    }
                    w.spawns.push_back(std::move(e));
                }
            }
            config.waves.push_back(std::move(w));
        }
    }

    // Catch dangling references (a wave naming a type that has no archetype) at
    // load time, where the message points at the config, rather than at spawn.
    for (std::size_t wi = 0; wi < config.waves.size(); ++wi) {
        for (const auto& e : config.waves[wi].spawns) {
            if (config.types.find(e.type) == config.types.end()) {
                std::cerr << "[EnemySpawnSystem] wave " << wi
                          << " references unknown enemy type '" << e.type << "'\n";
            }
        }
    }

    return config;
}
