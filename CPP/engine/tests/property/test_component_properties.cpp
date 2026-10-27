/**
 * Property-based tests for ComponentStorage
 *
 * These tests verify universal properties that should hold across all possible
 * sequences of component operations. Unlike unit tests that check specific examples,
 * property tests generate random operation sequences to ensure correctness under
 * arbitrary usage patterns.
 *
 * Requirements tested: 11.1, 11.3, 11.4, 11.5
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/entity_manager.hpp"
#include <vector>
#include <random>
#include <algorithm>

static constexpr int NUM_OUTER_TESTS = 10;
static constexpr int NUM_INNER_TESTS = 5;

static float rand_float(std::mt19937& gen) {
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    return dist(gen);
}

static uint8_t rand_byte(std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, 255);
    return static_cast<uint8_t>(dist(gen));
}

static bool positions_equal(const Position& a, const Position& b) {
    return a.x == b.x && a.y == b.y;
}

static bool sizes_equal(const Size& a, const Size& b) {
    return a.width == b.width && a.height == b.height;
}

static bool colors_equal(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

/**
 * Property: Component storage round-trip
 *
 * For any entity and any component value, after add_component(entity, component),
 * get_component(entity) must return a component equal to the one that was added.
 */
TEST_CASE("Property: component storage round-trip", "[component_storage]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;

    for (int i = 0; i < NUM_INNER_TESTS; ++i) {
        Entity e = em.create_entity();

        Position pos{rand_float(gen), rand_float(gen)};
        storage.add_component(e, pos);
        auto retrieved_pos = storage.get_component<Position>(e);
        REQUIRE(retrieved_pos.has_value());
        REQUIRE(positions_equal(retrieved_pos->get(), pos));

        Size size{rand_float(gen), rand_float(gen)};
        storage.add_component(e, size);
        auto retrieved_size = storage.get_component<Size>(e);
        REQUIRE(retrieved_size.has_value());
        REQUIRE(sizes_equal(retrieved_size->get(), size));

        Color color{rand_byte(gen), rand_byte(gen), rand_byte(gen), rand_byte(gen)};
        storage.add_component(e, color);
        auto retrieved_color = storage.get_component<Color>(e);
        REQUIRE(retrieved_color.has_value());
        REQUIRE(colors_equal(retrieved_color->get(), color));
    }
}

/**
 * Property: Component storage round-trip — edge cases
 *
 * Verifies round-trip holds for zero values, negative values, large values,
 * and corner-case color combinations.
 */
TEST_CASE("Property: component storage round-trip edge cases", "[component_storage]") {
    auto _iter = GENERATE(take(NUM_OUTER_TESTS, random(0, 1)));
    (void)_iter;

    EntityManager em;
    ComponentStorage storage;
    Entity e = em.create_entity();

    // Zero values
    {
        storage.add_component(e, Position{0.0f, 0.0f});
        auto r = storage.get_component<Position>(e);
        REQUIRE(r.has_value());
        REQUIRE(positions_equal(r->get(), Position{0.0f, 0.0f}));

        storage.add_component(e, Color{0, 0, 0, 0});
        auto rc = storage.get_component<Color>(e);
        REQUIRE(rc.has_value());
        REQUIRE(colors_equal(rc->get(), Color{0, 0, 0, 0}));
    }

    // Negative values
    {
        storage.add_component(e, Position{-999.9f, -888.8f});
        auto r = storage.get_component<Position>(e);
        REQUIRE(r.has_value());
        REQUIRE(positions_equal(r->get(), Position{-999.9f, -888.8f}));
    }

    // Large values
    {
        storage.add_component(e, Position{10000.0f, 20000.0f});
        auto r = storage.get_component<Position>(e);
        REQUIRE(r.has_value());
        REQUIRE(positions_equal(r->get(), Position{10000.0f, 20000.0f}));
    }

    // Maximum color values
    {
        storage.add_component(e, Color{255, 255, 255, 255});
        auto rc = storage.get_component<Color>(e);
        REQUIRE(rc.has_value());
        REQUIRE(colors_equal(rc->get(), Color{255, 255, 255, 255}));
    }

    em.destroy_entity(e);
}

/**
 * Property: Component add-remove round-trip
 *
 * For any entity, add_component<T>() followed by remove_component<T>() must
 * return the entity to its previous state (get_component returns nullopt).
 */
TEST_CASE("Property: component add-remove round-trip", "[component_storage]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;

    for (int i = 0; i < NUM_INNER_TESTS; ++i) {
        Entity e = em.create_entity();

        REQUIRE_FALSE(storage.has_component<Position>(e));
        REQUIRE_FALSE(storage.has_component<Size>(e));
        REQUIRE_FALSE(storage.has_component<Color>(e));

        Position pos{rand_float(gen), rand_float(gen)};
        storage.add_component(e, pos);
        REQUIRE(storage.has_component<Position>(e));
        storage.remove_component<Position>(e);
        REQUIRE_FALSE(storage.has_component<Position>(e));
        REQUIRE_FALSE(storage.get_component<Position>(e).has_value());

        Size size{rand_float(gen), rand_float(gen)};
        storage.add_component(e, size);
        REQUIRE(storage.has_component<Size>(e));
        storage.remove_component<Size>(e);
        REQUIRE_FALSE(storage.has_component<Size>(e));
        REQUIRE_FALSE(storage.get_component<Size>(e).has_value());

        Color color{rand_byte(gen), rand_byte(gen), rand_byte(gen), rand_byte(gen)};
        storage.add_component(e, color);
        REQUIRE(storage.has_component<Color>(e));
        storage.remove_component<Color>(e);
        REQUIRE_FALSE(storage.has_component<Color>(e));
        REQUIRE_FALSE(storage.get_component<Color>(e).has_value());
    }
}

/**
 * Property: Component add-remove with multiple cycles
 *
 * Multiple add-remove cycles on the same entity must not corrupt state.
 */
TEST_CASE("Property: component add-remove multiple cycles", "[component_storage]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;
    Entity e = em.create_entity();

    for (int cycle = 0; cycle < NUM_INNER_TESTS; ++cycle) {
        REQUIRE_FALSE(storage.has_component<Position>(e));

        Position pos{rand_float(gen), rand_float(gen)};
        storage.add_component(e, pos);
        REQUIRE(storage.has_component<Position>(e));

        auto retrieved = storage.get_component<Position>(e);
        REQUIRE(retrieved.has_value());
        REQUIRE(positions_equal(retrieved->get(), pos));

        storage.remove_component<Position>(e);
        REQUIRE_FALSE(storage.has_component<Position>(e));
        REQUIRE_FALSE(storage.get_component<Position>(e).has_value());
    }

    REQUIRE_FALSE(storage.has_component<Position>(e));
    REQUIRE_FALSE(storage.has_component<Size>(e));
    REQUIRE_FALSE(storage.has_component<Color>(e));
}

/**
 * Property: Component add-remove independence
 *
 * Add-remove operations on one component type must not affect other component
 * types on the same entity.
 */
TEST_CASE("Property: component add-remove independence", "[component_storage]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;
    Entity e = em.create_entity();

    Position pos{rand_float(gen), rand_float(gen)};
    Size size{rand_float(gen), rand_float(gen)};
    Color color{rand_byte(gen), rand_byte(gen), rand_byte(gen), rand_byte(gen)};

    storage.add_component(e, pos);
    storage.add_component(e, size);
    storage.add_component(e, color);

    REQUIRE(storage.has_component<Position>(e));
    REQUIRE(storage.has_component<Size>(e));
    REQUIRE(storage.has_component<Color>(e));

    storage.remove_component<Position>(e);
    REQUIRE_FALSE(storage.has_component<Position>(e));
    REQUIRE(storage.has_component<Size>(e));
    REQUIRE(storage.has_component<Color>(e));

    auto retrieved_size = storage.get_component<Size>(e);
    REQUIRE(retrieved_size.has_value());
    REQUIRE(sizes_equal(retrieved_size->get(), size));

    auto retrieved_color = storage.get_component<Color>(e);
    REQUIRE(retrieved_color.has_value());
    REQUIRE(colors_equal(retrieved_color->get(), color));

    storage.remove_component<Size>(e);
    REQUIRE_FALSE(storage.has_component<Position>(e));
    REQUIRE_FALSE(storage.has_component<Size>(e));
    REQUIRE(storage.has_component<Color>(e));

    retrieved_color = storage.get_component<Color>(e);
    REQUIRE(retrieved_color.has_value());
    REQUIRE(colors_equal(retrieved_color->get(), color));

    storage.remove_component<Color>(e);
    REQUIRE_FALSE(storage.has_component<Position>(e));
    REQUIRE_FALSE(storage.has_component<Size>(e));
    REQUIRE_FALSE(storage.has_component<Color>(e));
}

/**
 * Property: Component retrieval idempotence
 *
 * Calling get_component() twice in succession must return the same result both
 * times — component retrieval is a pure read with no side effects.
 */
TEST_CASE("Property: component retrieval idempotence", "[component_storage]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;

    for (int i = 0; i < NUM_INNER_TESTS; ++i) {
        Entity e = em.create_entity();
        storage.add_component(e, Position{rand_float(gen), rand_float(gen)});
        storage.add_component(e, Size{rand_float(gen), rand_float(gen)});
        storage.add_component(e, Color{rand_byte(gen), rand_byte(gen), rand_byte(gen), rand_byte(gen)});

        auto pos1 = storage.get_component<Position>(e);
        auto pos2 = storage.get_component<Position>(e);
        REQUIRE(pos1.has_value());
        REQUIRE(pos2.has_value());
        REQUIRE(positions_equal(pos1->get(), pos2->get()));

        auto size1 = storage.get_component<Size>(e);
        auto size2 = storage.get_component<Size>(e);
        REQUIRE(size1.has_value());
        REQUIRE(size2.has_value());
        REQUIRE(sizes_equal(size1->get(), size2->get()));

        auto color1 = storage.get_component<Color>(e);
        auto color2 = storage.get_component<Color>(e);
        REQUIRE(color1.has_value());
        REQUIRE(color2.has_value());
        REQUIRE(colors_equal(color1->get(), color2->get()));
    }
}

/**
 * Property: Component retrieval idempotence with modification
 *
 * After modifying a component through the returned reference, subsequent
 * get_component() calls must return the modified value consistently.
 */
TEST_CASE("Property: component retrieval idempotence with modification", "[component_storage]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);

    EntityManager em;
    ComponentStorage storage;
    Entity e = em.create_entity();

    Position initial_pos{rand_float(gen), rand_float(gen)};
    storage.add_component(e, initial_pos);

    auto pos_ref = storage.get_component<Position>(e);
    REQUIRE(pos_ref.has_value());

    float new_x = rand_float(gen);
    float new_y = rand_float(gen);
    pos_ref->get().x = new_x;
    pos_ref->get().y = new_y;

    auto pos1 = storage.get_component<Position>(e);
    auto pos2 = storage.get_component<Position>(e);
    REQUIRE(pos1.has_value());
    REQUIRE(pos2.has_value());
    REQUIRE(pos1->get().x == new_x);
    REQUIRE(pos1->get().y == new_y);
    REQUIRE(positions_equal(pos1->get(), pos2->get()));
}
