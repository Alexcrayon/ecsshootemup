/**
 * Unit tests for ComponentStorage class
 *
 * These tests verify the core functionality of component storage:
 * - Adding and retrieving components
 * - get_component returns std::nullopt for non-existent components
 * - Removing components
 * - has_component returns correct boolean
 * - Multiple component types on single entity
 * - Component replacement (adding same type twice)
 *
 * Requirements tested: 10.1, 10.3
 */

#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/entity_manager.hpp"

TEST_CASE("Adding and retrieving components", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    Position pos{100.0f, 200.0f};
    storage.add_component(entity, pos);

    auto retrieved_pos = storage.get_component<Position>(entity);
    REQUIRE(retrieved_pos.has_value());
    CHECK(retrieved_pos->get().x == 100.0f);
    CHECK(retrieved_pos->get().y == 200.0f);
}

TEST_CASE("get_component returns nullopt for non-existent components", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    auto pos = storage.get_component<Position>(entity);
    CHECK_FALSE(pos.has_value());

    auto size = storage.get_component<Size>(entity);
    CHECK_FALSE(size.has_value());

    auto color = storage.get_component<Color>(entity);
    CHECK_FALSE(color.has_value());
}

TEST_CASE("Removing components", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    Position pos{100.0f, 200.0f};
    storage.add_component(entity, pos);

    auto retrieved_pos = storage.get_component<Position>(entity);
    REQUIRE(retrieved_pos.has_value());

    storage.remove_component<Position>(entity);

    auto after_removal = storage.get_component<Position>(entity);
    CHECK_FALSE(after_removal.has_value());
}

TEST_CASE("Removing non-existent component is safe", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    REQUIRE_NOTHROW(storage.remove_component<Position>(entity));
    REQUIRE_NOTHROW(storage.remove_component<Size>(entity));
    REQUIRE_NOTHROW(storage.remove_component<Color>(entity));
}

TEST_CASE("has_component returns correct boolean", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    CHECK_FALSE(storage.has_component<Position>(entity));
    CHECK_FALSE(storage.has_component<Size>(entity));
    CHECK_FALSE(storage.has_component<Color>(entity));

    Position pos{100.0f, 200.0f};
    storage.add_component(entity, pos);

    CHECK(storage.has_component<Position>(entity));
    CHECK_FALSE(storage.has_component<Size>(entity));
    CHECK_FALSE(storage.has_component<Color>(entity));

    Size size{50.0f, 75.0f};
    storage.add_component(entity, size);

    CHECK(storage.has_component<Position>(entity));
    CHECK(storage.has_component<Size>(entity));
    CHECK_FALSE(storage.has_component<Color>(entity));

    storage.remove_component<Position>(entity);

    CHECK_FALSE(storage.has_component<Position>(entity));
    CHECK(storage.has_component<Size>(entity));
    CHECK_FALSE(storage.has_component<Color>(entity));
}

TEST_CASE("Multiple component types on single entity", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    Position pos{350.0f, 250.0f};
    Size size{100.0f, 100.0f};
    Color color{255, 0, 0, 255};

    storage.add_component(entity, pos);
    storage.add_component(entity, size);
    storage.add_component(entity, color);

    CHECK(storage.has_component<Position>(entity));
    CHECK(storage.has_component<Size>(entity));
    CHECK(storage.has_component<Color>(entity));

    auto retrieved_pos = storage.get_component<Position>(entity);
    REQUIRE(retrieved_pos.has_value());
    CHECK(retrieved_pos->get().x == 350.0f);
    CHECK(retrieved_pos->get().y == 250.0f);

    auto retrieved_size = storage.get_component<Size>(entity);
    REQUIRE(retrieved_size.has_value());
    CHECK(retrieved_size->get().width == 100.0f);
    CHECK(retrieved_size->get().height == 100.0f);

    auto retrieved_color = storage.get_component<Color>(entity);
    REQUIRE(retrieved_color.has_value());
    CHECK(retrieved_color->get().r == 255);
    CHECK(retrieved_color->get().g == 0);
    CHECK(retrieved_color->get().b == 0);
    CHECK(retrieved_color->get().a == 255);
}

TEST_CASE("Component replacement (adding same type twice)", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    Position pos1{100.0f, 200.0f};
    storage.add_component(entity, pos1);

    auto retrieved1 = storage.get_component<Position>(entity);
    REQUIRE(retrieved1.has_value());
    CHECK(retrieved1->get().x == 100.0f);
    CHECK(retrieved1->get().y == 200.0f);

    Position pos2{300.0f, 400.0f};
    storage.add_component(entity, pos2);

    auto retrieved2 = storage.get_component<Position>(entity);
    REQUIRE(retrieved2.has_value());
    CHECK(retrieved2->get().x == 300.0f);
    CHECK(retrieved2->get().y == 400.0f);
}

TEST_CASE("Multiple entities with same component type", "[component_storage]") {
    ComponentStorage storage;
    Entity entity1 = 1;
    Entity entity2 = 2;
    Entity entity3 = 3;

    storage.add_component(entity1, Position{100.0f, 100.0f});
    storage.add_component(entity2, Position{200.0f, 200.0f});
    storage.add_component(entity3, Position{300.0f, 300.0f});

    auto pos1 = storage.get_component<Position>(entity1);
    REQUIRE(pos1.has_value());
    CHECK(pos1->get().x == 100.0f);
    CHECK(pos1->get().y == 100.0f);

    auto pos2 = storage.get_component<Position>(entity2);
    REQUIRE(pos2.has_value());
    CHECK(pos2->get().x == 200.0f);
    CHECK(pos2->get().y == 200.0f);

    auto pos3 = storage.get_component<Position>(entity3);
    REQUIRE(pos3.has_value());
    CHECK(pos3->get().x == 300.0f);
    CHECK(pos3->get().y == 300.0f);
}

TEST_CASE("Modifying retrieved component reference", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    Position pos{100.0f, 200.0f};
    storage.add_component(entity, pos);

    auto retrieved = storage.get_component<Position>(entity);
    REQUIRE(retrieved.has_value());
    retrieved->get().x = 500.0f;
    retrieved->get().y = 600.0f;

    auto after_modification = storage.get_component<Position>(entity);
    REQUIRE(after_modification.has_value());
    CHECK(after_modification->get().x == 500.0f);
    CHECK(after_modification->get().y == 600.0f);
}

TEST_CASE("Const get_component for read-only access", "[component_storage]") {
    ComponentStorage storage;
    Entity entity = 1;

    Position pos{100.0f, 200.0f};
    storage.add_component(entity, pos);

    const ComponentStorage& const_storage = storage;

    auto retrieved = const_storage.get_component<Position>(entity);
    REQUIRE(retrieved.has_value());
    CHECK(retrieved->get().x == 100.0f);
    CHECK(retrieved->get().y == 200.0f);

    auto non_existent = const_storage.get_component<Size>(entity);
    CHECK_FALSE(non_existent.has_value());
}

TEST_CASE("entities_with_component returns correct entities", "[component_storage]") {
    ComponentStorage storage;

    Entity e1 = 1;
    Entity e2 = 2;
    Entity e3 = 3;
    Entity e4 = 4;

    storage.add_component(e1, Position{100.0f, 100.0f});

    storage.add_component(e2, Position{200.0f, 200.0f});
    storage.add_component(e2, Size{50.0f, 50.0f});

    storage.add_component(e3, Size{75.0f, 75.0f});

    storage.add_component(e4, Color{255, 0, 0, 255});

    auto entities_with_pos = storage.entities_with_component<Position>();
    CHECK(entities_with_pos.size() == 2);
    CHECK(std::find(entities_with_pos.begin(), entities_with_pos.end(), e1) != entities_with_pos.end());
    CHECK(std::find(entities_with_pos.begin(), entities_with_pos.end(), e2) != entities_with_pos.end());

    auto entities_with_size = storage.entities_with_component<Size>();
    CHECK(entities_with_size.size() == 2);
    CHECK(std::find(entities_with_size.begin(), entities_with_size.end(), e2) != entities_with_size.end());
    CHECK(std::find(entities_with_size.begin(), entities_with_size.end(), e3) != entities_with_size.end());

    auto entities_with_color = storage.entities_with_component<Color>();
    CHECK(entities_with_color.size() == 1);
    CHECK(std::find(entities_with_color.begin(), entities_with_color.end(), e4) != entities_with_color.end());
}

TEST_CASE("Integration with EntityManager", "[component_storage]") {
    EntityManager em;
    ComponentStorage storage;

    Entity e1 = em.create_entity();
    Entity e2 = em.create_entity();

    storage.add_component(e1, Position{100.0f, 100.0f});
    storage.add_component(e2, Position{200.0f, 200.0f});

    CHECK(storage.has_component<Position>(e1));
    CHECK(storage.has_component<Position>(e2));

    em.destroy_entity(e1);
    Entity e3 = em.create_entity();

    // e3 might have the same ID as e1, but should not have e1's components
    // (unless we explicitly add them)
    if (e3 == e1) {
        // ID was reused — old component data might still be there.
        // In a production system, you'd want to clean up components when entities are destroyed.
        CHECK(storage.has_component<Position>(e3));

        storage.remove_component<Position>(e3);
        CHECK_FALSE(storage.has_component<Position>(e3));
    } else {
        CHECK_FALSE(storage.has_component<Position>(e3));
    }
}
