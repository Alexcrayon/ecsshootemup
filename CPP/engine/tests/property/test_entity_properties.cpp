/**
 * Property-based tests for EntityManager
 *
 * These tests verify universal properties that should hold across all possible
 * sequences of entity operations. Unlike unit tests that check specific examples,
 * property tests generate random operation sequences to ensure correctness under
 * arbitrary usage patterns.
 *
 * Requirements tested: 11.1, 11.2, 11.5
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "engine/ecs/entity_manager.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>

static constexpr int NUM_OUTER_TESTS = 10;
static constexpr int NUM_INNER_TESTS = 5;

/**
 * Property: Entity count invariant
 *
 * For any sequence of create_entity() and destroy_entity() operations,
 * active_count() must equal creates minus destroys.
 */
TEST_CASE("Property: entity count invariant", "[entity_manager]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> op_dist(0, 1);

    EntityManager em;
    std::vector<Entity> entities;
    int creates = 0;
    int destroys = 0;

    for (int op = 0; op < NUM_INNER_TESTS * 4; ++op) {
        int operation = op_dist(gen);

        if (operation == 0 || entities.empty()) {
            Entity e = em.create_entity();
            entities.push_back(e);
            creates++;
            REQUIRE(em.active_count() == static_cast<size_t>(creates - destroys));
        } else {
            std::uniform_int_distribution<> entity_dist(0, static_cast<int>(entities.size()) - 1);
            int idx = entity_dist(gen);
            Entity e = entities[idx];
            em.destroy_entity(e);
            entities.erase(entities.begin() + idx);
            destroys++;
            REQUIRE(em.active_count() == static_cast<size_t>(creates - destroys));
        }
    }

    REQUIRE(em.active_count() == static_cast<size_t>(creates - destroys));
    REQUIRE(entities.size() == static_cast<size_t>(em.active_count()));
}

/**
 * Property: Entity count invariant — edge cases
 *
 * Verifies bulk-create/destroy, alternating, and double-destroy patterns.
 */
TEST_CASE("Property: entity count invariant edge cases", "[entity_manager]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;

    // Edge Case 1: Create many entities, then destroy all in random order
    {
        std::vector<Entity> entities;
        for (int i = 0; i < NUM_INNER_TESTS * 2; ++i) {
            entities.push_back(em.create_entity());
        }
        REQUIRE(em.active_count() == static_cast<size_t>(NUM_INNER_TESTS * 2));

        std::shuffle(entities.begin(), entities.end(), gen);
        for (int i = 0; i < NUM_INNER_TESTS * 2; ++i) {
            em.destroy_entity(entities[i]);
            REQUIRE(em.active_count() == static_cast<size_t>(NUM_INNER_TESTS * 2 - i - 1));
        }
        REQUIRE(em.active_count() == 0);
    }

    // Edge Case 2: Alternating create/destroy
    {
        for (int i = 0; i < NUM_INNER_TESTS; ++i) {
            Entity e = em.create_entity();
            REQUIRE(em.active_count() == 1);
            em.destroy_entity(e);
            REQUIRE(em.active_count() == 0);
        }
    }

    // Edge Case 3: Multiple destroys of same entity (should be safe no-ops)
    {
        Entity e = em.create_entity();
        REQUIRE(em.active_count() == 1);
        em.destroy_entity(e);
        REQUIRE(em.active_count() == 0);
        em.destroy_entity(e);
        REQUIRE(em.active_count() == 0);
    }

    // Edge Case 4: Create, destroy some, create more, verify count
    {
        std::vector<Entity> entities;
        for (int i = 0; i < NUM_INNER_TESTS * 2; ++i) {
            entities.push_back(em.create_entity());
        }
        REQUIRE(em.active_count() == static_cast<size_t>(NUM_INNER_TESTS * 2));

        std::shuffle(entities.begin(), entities.end(), gen);
        for (int i = 0; i < NUM_INNER_TESTS; ++i) {
            em.destroy_entity(entities[i]);
        }
        entities.erase(entities.begin(), entities.begin() + NUM_INNER_TESTS);
        REQUIRE(em.active_count() == NUM_INNER_TESTS);

        for (int i = 0; i < NUM_INNER_TESTS; ++i) {
            entities.push_back(em.create_entity());
        }
        REQUIRE(em.active_count() == static_cast<size_t>(NUM_INNER_TESTS * 2));

        for (Entity e : entities) {
            em.destroy_entity(e);
        }
        REQUIRE(em.active_count() == 0);
    }
}

/**
 * Property: Entity ID uniqueness
 *
 * For any sequence of entity creation operations, all returned entity IDs
 * must be unique among currently active entities.
 */
TEST_CASE("Property: entity ID uniqueness", "[entity_manager]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> op_dist(0, 1);

    EntityManager em;
    std::vector<Entity> active_entities;
    std::unordered_set<Entity> active_ids;

    for (int op = 0; op < NUM_INNER_TESTS * 10; ++op) {
        int operation = op_dist(gen);

        if (operation == 0 || active_entities.empty()) {
            Entity e = em.create_entity();

            REQUIRE(active_ids.count(e) == 0);
            REQUIRE(em.is_alive(e));

            active_entities.push_back(e);
            active_ids.insert(e);

            REQUIRE(active_ids.size() == active_entities.size());
        } else {
            std::uniform_int_distribution<> entity_dist(0, static_cast<int>(active_entities.size()) - 1);
            int idx = entity_dist(gen);
            Entity e = active_entities[idx];

            REQUIRE(em.is_alive(e));
            em.destroy_entity(e);
            REQUIRE_FALSE(em.is_alive(e));

            active_entities.erase(active_entities.begin() + idx);
            active_ids.erase(e);

            REQUIRE(active_ids.size() == active_entities.size());
        }
    }

    REQUIRE(active_ids.size() == active_entities.size());
    REQUIRE(em.active_count() == active_entities.size());

    for (Entity e : active_entities) {
        REQUIRE(em.is_alive(e));
    }
}

/**
 * Property: Entity ID uniqueness with ID reuse
 *
 * Destroyed entity IDs can be reused for new entities, but at any point in
 * time all active entities have unique IDs.
 */
TEST_CASE("Property: entity ID uniqueness with ID reuse", "[entity_manager]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;

    // Pattern 1: Create N entities, destroy all, create N more
    {
        std::vector<Entity> first_batch;
        std::unordered_set<Entity> first_batch_ids;

        for (int i = 0; i < NUM_INNER_TESTS * 2; ++i) {
            Entity e = em.create_entity();
            REQUIRE(first_batch_ids.count(e) == 0);
            first_batch.push_back(e);
            first_batch_ids.insert(e);
        }

        for (Entity e : first_batch) {
            em.destroy_entity(e);
            REQUIRE_FALSE(em.is_alive(e));
        }
        REQUIRE(em.active_count() == 0);

        std::unordered_set<Entity> second_batch_ids;
        for (int i = 0; i < NUM_INNER_TESTS * 2; ++i) {
            Entity e = em.create_entity();
            REQUIRE(second_batch_ids.count(e) == 0);
            REQUIRE(em.is_alive(e));
            second_batch_ids.insert(e);
        }
        REQUIRE(em.active_count() == static_cast<size_t>(NUM_INNER_TESTS * 2));

        for (Entity e : second_batch_ids) {
            em.destroy_entity(e);
        }
    }

    // Pattern 2: Interleaved create/destroy with uniqueness checks
    {
        std::vector<Entity> active;
        std::unordered_set<Entity> active_ids;

        for (int cycle = 0; cycle < NUM_INNER_TESTS; ++cycle) {
            for (int i = 0; i < 3; ++i) {
                Entity e = em.create_entity();
                REQUIRE(active_ids.count(e) == 0);
                active.push_back(e);
                active_ids.insert(e);
            }

            std::shuffle(active.begin(), active.end(), gen);
            for (int i = 0; i < 2 && !active.empty(); ++i) {
                Entity e = active.back();
                active.pop_back();
                active_ids.erase(e);
                em.destroy_entity(e);
            }

            REQUIRE(active_ids.size() == active.size());
            REQUIRE(em.active_count() == active.size());
        }

        for (Entity e : active) {
            em.destroy_entity(e);
        }
    }
}
