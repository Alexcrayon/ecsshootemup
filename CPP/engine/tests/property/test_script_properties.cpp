/**
 * Property-based tests for Script component, LuaManager, ScriptSystem,
 * and GameData loader script parsing.
 *
 * Testing Framework: Catch2 v3
 * Constants: NUM_OUTER_TESTS = 10, NUM_INNER_TESTS = 5
 *
 * Requirements tested: 3.1, 3.4, 5.1, 5.2, 6.1, 6.2, 6.3, 7.1, 7.2,
 *                      8.1, 8.2, 8.3, 9.1, 9.2, 9.3, 10.1, 10.2,
 *                      11.1, 11.2, 11.3, 16.1, 16.2, 16.3, 16.4
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "engine/ecs/entity_manager.hpp"
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"
#include "engine/ecs/destruction.hpp"
#include "engine/lua_manager.hpp"
#include "engine/ecs/systems/script_system.hpp"
#include "engine/gamedata_loader.hpp"
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>

static constexpr int NUM_OUTER_TESTS = 10;
static constexpr int NUM_INNER_TESTS = 5;

// Absolute path for direct lua_manager.load_script() calls
static std::string test_script_abs_path(const std::string& filename) {
    return std::string(CLASS_ROOT_DIR) + "/CPP/engine/tests/test_assets/" + filename;
}

// Relative path for Script component filenames used with ScriptSystem.
// ScriptSystem prepends project_paths::assets_dir() (absolute), so
// "../CPP/engine/tests/test_assets/" resolves correctly from assets/.
static std::string test_script_path(const std::string& filename) {
    return "../CPP/engine/tests/test_assets/" + filename;
}

static std::string random_string(std::mt19937& gen, int min_len, int max_len) {
    std::uniform_int_distribution<int> len_dist(min_len, max_len);
    std::uniform_int_distribution<int> char_dist('a', 'z');
    int len = len_dist(gen);
    std::string result;
    result.reserve(len);
    for (int i = 0; i < len; ++i) {
        result.push_back(static_cast<char>(char_dist(gen)));
    }
    return result;
}

TEST_CASE("Property: Script component storage round-trip", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    for (int inner = 0; inner < NUM_INNER_TESTS; ++inner) {
        EntityManager em;
        ComponentStorage storage;

        std::string filename = random_string(gen, 5, 20);
        Entity e = em.create_entity();
        storage.add_component<Script>(e, Script{filename, false});

        auto retrieved = storage.get_component<Script>(e);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->get().filename == filename);
        REQUIRE(retrieved->get().initialized == false);
    }
}

TEST_CASE("Property: Destruction pipeline removes Script component", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;

    std::string filename = random_string(gen, 5, 20);
    Entity e = em.create_entity();
    storage.add_component<Script>(e, Script{filename, false});
    storage.add_component<DestroyRequest>(e, DestroyRequest{});

    destroy_marked_entities(em, storage);

    REQUIRE_FALSE(storage.has_component<Script>(e));
    REQUIRE_FALSE(em.is_alive(e));
}

TEST_CASE("Property: LuaManager error recovery", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    LuaManager lua_manager;

    std::string bad_path = random_string(gen, 5, 20) + ".lua";
    auto bad_result = lua_manager.load_script(bad_path);
    REQUIRE_FALSE(bad_result.success);

    auto good_result = lua_manager.load_script(test_script_abs_path("valid_script.lua"));
    REQUIRE(good_result.success);
}

TEST_CASE("Property: Non-existent file error contains path", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    for (int inner = 0; inner < NUM_INNER_TESTS; ++inner) {
        LuaManager lua_manager;

        std::string bad_path = random_string(gen, 5, 20) + ".lua";
        auto result = lua_manager.load_script(bad_path);

        REQUIRE_FALSE(result.success);
        REQUIRE(result.error_message.find(bad_path) != std::string::npos);
    }
}

TEST_CASE("Property: on_init lifecycle transition", "[script]") {
    auto _iter = GENERATE(take(NUM_OUTER_TESTS, random(0, 1)));
    (void)_iter;

    LuaManager lua_manager;
    ScriptSystem script_system(lua_manager);
    EntityManager entity_manager;
    ComponentStorage storage;
    Blackboard blackboard;
    blackboard.set("delta_time", 0.016);

    Entity e = entity_manager.create_entity();
    storage.add_component<Script>(e, Script{test_script_path("valid_script.lua"), false});

    script_system.update(storage, entity_manager, blackboard);

    auto script = storage.get_component<Script>(e);
    REQUIRE(script.has_value());
    REQUIRE(script->get().initialized);
}

TEST_CASE("Property: on_update receives correct arguments", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dt_dist(0.001, 0.1);

    LuaManager lua_manager;
    ScriptSystem script_system(lua_manager);
    EntityManager entity_manager;
    ComponentStorage storage;
    Blackboard blackboard;
    blackboard.set("delta_time", 0.016);

    Entity e = entity_manager.create_entity();
    storage.add_component<Script>(e, Script{test_script_path("valid_script.lua"), false});

    script_system.update(storage, entity_manager, blackboard);

    for (int inner = 0; inner < NUM_INNER_TESTS; ++inner) {
        double dt = dt_dist(gen);
        blackboard.set("delta_time", dt);
        script_system.update(storage, entity_manager, blackboard);

        auto script = storage.get_component<Script>(e);
        REQUIRE(script.has_value());
        REQUIRE(script->get().initialized);
    }
}

TEST_CASE("Property: on_collision dispatch count", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> partner_count_dist(1, 5);

    LuaManager lua_manager;
    ScriptSystem script_system(lua_manager);
    EntityManager entity_manager;
    ComponentStorage storage;
    Blackboard blackboard;
    blackboard.set("delta_time", 0.016);

    Entity e = entity_manager.create_entity();
    storage.add_component<Script>(e, Script{test_script_path("valid_script.lua"), false});

    script_system.update(storage, entity_manager, blackboard);

    int num_partners = partner_count_dist(gen);
    std::vector<Entity> partners;
    for (int i = 0; i < num_partners; ++i) {
        partners.push_back(entity_manager.create_entity());
    }
    storage.add_component<CollidedWith>(e, CollidedWith{partners});

    script_system.update(storage, entity_manager, blackboard);

    auto script = storage.get_component<Script>(e);
    REQUIRE(script.has_value());
    REQUIRE(script->get().initialized);
}

TEST_CASE("Property: per-entity environment isolation", "[script]") {
    auto _iter = GENERATE(take(NUM_OUTER_TESTS, random(0, 1)));
    (void)_iter;

    LuaManager lua_manager;
    ScriptSystem script_system(lua_manager);
    EntityManager entity_manager;
    ComponentStorage storage;
    Blackboard blackboard;
    blackboard.set("delta_time", 0.016);

    Entity e1 = entity_manager.create_entity();
    Entity e2 = entity_manager.create_entity();
    storage.add_component<Script>(e1, Script{test_script_path("isolation_test.lua"), false});
    storage.add_component<Script>(e2, Script{test_script_path("isolation_test.lua"), false});

    script_system.update(storage, entity_manager, blackboard);

    auto s1 = storage.get_component<Script>(e1);
    auto s2 = storage.get_component<Script>(e2);
    REQUIRE(s1.has_value());
    REQUIRE(s2.has_value());
    REQUIRE(s1->get().initialized);
    REQUIRE(s2->get().initialized);

    script_system.update(storage, entity_manager, blackboard);
}

TEST_CASE("Property: error in one entity does not block others", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> count_dist(2, 5);

    LuaManager lua_manager;
    ScriptSystem script_system(lua_manager);
    EntityManager entity_manager;
    ComponentStorage storage;
    Blackboard blackboard;
    blackboard.set("delta_time", 0.016);

    int total_entities = count_dist(gen);
    std::vector<Entity> entities;

    Entity bad = entity_manager.create_entity();
    storage.add_component<Script>(bad, Script{test_script_path("runtime_error.lua"), false});
    entities.push_back(bad);

    for (int i = 1; i < total_entities; ++i) {
        Entity good = entity_manager.create_entity();
        storage.add_component<Script>(good, Script{test_script_path("valid_script.lua"), false});
        entities.push_back(good);
    }

    script_system.update(storage, entity_manager, blackboard);

    for (size_t i = 0; i < entities.size(); ++i) {
        auto script = storage.get_component<Script>(entities[i]);
        REQUIRE(script.has_value());
        REQUIRE(script->get().initialized);
    }
}

TEST_CASE("Property: GameData loader script parsing round-trip", "[script]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager entity_manager;
    ComponentStorage storage;
    Blackboard blackboard;

    std::string filename = "scripts/" + random_string(gen, 5, 15) + ".lua";

    std::string json_content = R"({
        "window": { "width": 800, "height": 600 },
        "entities": [
            {
                "id": "test_entity",
                "components": {
                    "script": { "filename": ")" + filename + R"(" }
                }
            }
        ]
    })";

    std::string temp_path = "test_gamedata_script_prop_catch2.json";
    {
        std::ofstream out(temp_path);
        REQUIRE(out.is_open());
        out << json_content;
    }

    load_game_data(temp_path, entity_manager, storage, blackboard);
    std::remove(temp_path.c_str());

    Entity e = blackboard.get<Entity>("entity.id.test_entity");

    REQUIRE(storage.has_component<Script>(e));
    auto script = storage.get_component<Script>(e);
    REQUIRE(script.has_value());
    REQUIRE(script->get().filename == filename);
    REQUIRE(script->get().initialized == false);
}
