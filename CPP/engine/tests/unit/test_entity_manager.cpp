/**
 * Unit tests for EntityManager class
 *
 * These tests verify the core functionality of entity lifecycle management:
 * - Entity creation returns valid IDs
 * - Entity destruction marks entities as inactive
 * - is_alive() correctly reports entity status
 * - Entity IDs are reused after destruction
 * - active_count() accurately tracks the number of active entities
 *
 * Requirements tested: 10.1, 10.2
 */

#include <catch2/catch_test_macros.hpp>
#include "engine/ecs/entity_manager.hpp"
#include <unordered_set>

TEST_CASE("Entity creation returns valid IDs", "[entity_manager]") {
    EntityManager em;

    Entity e1 = em.create_entity();
    CHECK(em.is_alive(e1));
    CHECK(em.active_count() == 1);

    Entity e2 = em.create_entity();
    CHECK(em.is_alive(e2));
    CHECK(em.active_count() == 2);

    CHECK(em.is_alive(e1));
    CHECK(em.is_alive(e2));
    CHECK(e1 != e2);
}

TEST_CASE("Entity destruction marks entity as inactive", "[entity_manager]") {
    EntityManager em;

    Entity e = em.create_entity();
    CHECK(em.is_alive(e));
    CHECK(em.active_count() == 1);

    em.destroy_entity(e);
    CHECK_FALSE(em.is_alive(e));
    CHECK(em.active_count() == 0);
}

TEST_CASE("is_alive returns correct status", "[entity_manager]") {
    EntityManager em;

    Entity e1 = em.create_entity();
    CHECK(em.is_alive(e1));

    em.destroy_entity(e1);
    CHECK_FALSE(em.is_alive(e1));

    Entity e2 = em.create_entity();
    CHECK(em.is_alive(e2));

    // Note: e1 and e2 may have the same ID due to reuse, so we can't
    // reliably test e1's status after e2 is created
}

TEST_CASE("ID reuse after destruction", "[entity_manager]") {
    EntityManager em;

    Entity e1 = em.create_entity();
    Entity original_id = e1;
    em.destroy_entity(e1);
    CHECK_FALSE(em.is_alive(original_id));

    Entity e2 = em.create_entity();
    CHECK(e2 == original_id);
    CHECK(em.is_alive(e2));

    // Note: e1 and e2 have the same ID value, so is_alive(e1) would return true
    // because it's checking the ID, not the "entity instance"
}

TEST_CASE("ID reuse with multiple entities", "[entity_manager]") {
    EntityManager em;

    Entity e1 = em.create_entity();
    Entity e2 = em.create_entity();
    Entity e3 = em.create_entity();

    CHECK(em.active_count() == 3);

    em.destroy_entity(e2);
    CHECK(em.active_count() == 2);
    CHECK(em.is_alive(e1));
    CHECK_FALSE(em.is_alive(e2));
    CHECK(em.is_alive(e3));

    Entity e4 = em.create_entity();
    CHECK(e4 == e2);
    CHECK(em.active_count() == 3);
}

TEST_CASE("active_count tracks entity count correctly", "[entity_manager]") {
    EntityManager em;

    CHECK(em.active_count() == 0);

    Entity e1 = em.create_entity();
    CHECK(em.active_count() == 1);

    Entity e2 = em.create_entity();
    CHECK(em.active_count() == 2);

    Entity e3 = em.create_entity();
    CHECK(em.active_count() == 3);

    em.destroy_entity(e1);
    CHECK(em.active_count() == 2);

    em.destroy_entity(e2);
    CHECK(em.active_count() == 1);

    em.destroy_entity(e3);
    CHECK(em.active_count() == 0);
}

TEST_CASE("Destroying non-existent entity is safe", "[entity_manager]") {
    EntityManager em;

    Entity e = em.create_entity();
    em.destroy_entity(e);

    REQUIRE_NOTHROW(em.destroy_entity(e));
    CHECK(em.active_count() == 0);

    Entity fake_entity = 9999;
    REQUIRE_NOTHROW(em.destroy_entity(fake_entity));
    CHECK(em.active_count() == 0);
}

TEST_CASE("Multiple creates and destroys maintain count", "[entity_manager]") {
    EntityManager em;

    std::vector<Entity> entities;

    for (int i = 0; i < 10; ++i) {
        entities.push_back(em.create_entity());
    }
    CHECK(em.active_count() == 10);

    for (int i = 0; i < 5; ++i) {
        em.destroy_entity(entities[i]);
    }
    CHECK(em.active_count() == 5);

    for (int i = 0; i < 3; ++i) {
        entities.push_back(em.create_entity());
    }
    CHECK(em.active_count() == 8);

    for (size_t i = 5; i < entities.size(); ++i) {
        em.destroy_entity(entities[i]);
    }
    CHECK(em.active_count() == 0);
}

TEST_CASE("Entity IDs are unique among active entities", "[entity_manager]") {
    EntityManager em;
    std::unordered_set<Entity> created_ids;

    for (int i = 0; i < 20; ++i) {
        Entity e = em.create_entity();
        CHECK(created_ids.find(e) == created_ids.end());
        created_ids.insert(e);
    }

    CHECK(em.active_count() == 20);
    CHECK(created_ids.size() == 20);
}

TEST_CASE("Create-destroy cycle", "[entity_manager]") {
    EntityManager em;

    for (int cycle = 0; cycle < 5; ++cycle) {
        std::vector<Entity> entities;
        for (int i = 0; i < 5; ++i) {
            entities.push_back(em.create_entity());
        }
        CHECK(em.active_count() == 5);

        for (Entity e : entities) {
            CHECK(em.is_alive(e));
        }

        for (Entity e : entities) {
            em.destroy_entity(e);
        }
        CHECK(em.active_count() == 0);

        for (Entity e : entities) {
            CHECK_FALSE(em.is_alive(e));
        }
    }
}
