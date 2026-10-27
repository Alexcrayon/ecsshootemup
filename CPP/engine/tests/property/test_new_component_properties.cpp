/**
 * Property-based tests for new component types and destruction pipeline
 *
 * These tests verify universal properties for the five new component types
 * (Rotation, Collider, Lifetime, WrapAround, DestroyRequest)
 * and the destroy_marked_entities() destruction pipeline.
 *
 * Testing Framework: Catch2 v3
 * Constants: NUM_OUTER_TESTS = 10, NUM_INNER_TESTS = 5
 *
 * Requirements tested: 1.1, 1.2, 2.1, 2.2, 2.3, 3.1, 4.1, 4.2,
 *                      6.1, 6.2, 7.1, 8.2, 8.3, 8.4, 9.1, 9.2, 9.3,
 *                      12.5, 12.6, 12.7
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "engine/ecs/entity_manager.hpp"
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/destruction.hpp"
#include <random>
#include <vector>
#include <algorithm>

static constexpr int NUM_OUTER_TESTS = 10;
static constexpr int NUM_INNER_TESTS = 5;

TEST_CASE("Property: Rotation component round-trip", "[new_component][rotation]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> float_dist(-1000.0f, 1000.0f);

    for (int inner = 0; inner < NUM_INNER_TESTS; ++inner) {
        EntityManager em;
        ComponentStorage storage;

        float angle = float_dist(gen);
        float angular_velocity = float_dist(gen);

        Entity e = em.create_entity();
        storage.add_component(e, Rotation{angle, angular_velocity});

        auto retrieved = storage.get_component<Rotation>(e);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->get().angle == angle);
        REQUIRE(retrieved->get().angular_velocity == angular_velocity);
    }
}

TEST_CASE("Property: Collider component round-trip", "[new_component][collider]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> float_dist(-1000.0f, 1000.0f);
    std::uniform_int_distribution<int> byte_dist(0, 255);

    for (int inner = 0; inner < NUM_INNER_TESTS; ++inner) {
        EntityManager em;
        ComponentStorage storage;

        float width = float_dist(gen);
        float height = float_dist(gen);
        uint8_t layer = static_cast<uint8_t>(byte_dist(gen));
        uint8_t mask = static_cast<uint8_t>(byte_dist(gen));

        Entity e = em.create_entity();
        storage.add_component(e, Collider{width, height, layer, mask});

        auto retrieved = storage.get_component<Collider>(e);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->get().width == width);
        REQUIRE(retrieved->get().height == height);
        REQUIRE(retrieved->get().layer == layer);
        REQUIRE(retrieved->get().mask == mask);
    }
}

TEST_CASE("Property: Lifetime component round-trip", "[new_component][lifetime]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> float_dist(-1000.0f, 1000.0f);

    for (int inner = 0; inner < NUM_INNER_TESTS; ++inner) {
        EntityManager em;
        ComponentStorage storage;

        float remaining = float_dist(gen);

        Entity e = em.create_entity();
        storage.add_component(e, Lifetime{remaining});

        auto retrieved = storage.get_component<Lifetime>(e);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->get().remaining == remaining);
    }
}

TEST_CASE("Property: Tag component add-then-has round-trip", "[new_component][tag]") {
    auto _iter = GENERATE(take(NUM_OUTER_TESTS, random(0, 1)));
    (void)_iter;

    EntityManager em;
    ComponentStorage storage;

    Entity e = em.create_entity();

    // WrapAround: add -> has == true, remove -> has == false
    REQUIRE_FALSE(storage.has_component<WrapAround>(e));
    storage.add_component(e, WrapAround{});
    REQUIRE(storage.has_component<WrapAround>(e));
    storage.remove_component<WrapAround>(e);
    REQUIRE_FALSE(storage.has_component<WrapAround>(e));

    // DestroyRequest: add -> has == true, remove -> has == false
    REQUIRE_FALSE(storage.has_component<DestroyRequest>(e));
    storage.add_component(e, DestroyRequest{});
    REQUIRE(storage.has_component<DestroyRequest>(e));
    storage.remove_component<DestroyRequest>(e);
    REQUIRE_FALSE(storage.has_component<DestroyRequest>(e));
}

TEST_CASE("Property: Destruction pipeline cleans up marked entities completely", "[new_component][destruction]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> float_dist(-1000.0f, 1000.0f);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<int> coin(0, 1);

    EntityManager em;
    ComponentStorage storage;

    std::vector<Entity> all_entities;
    std::vector<Entity> marked_entities;

    for (int i = 0; i < NUM_INNER_TESTS; ++i) {
        Entity e = em.create_entity();
        all_entities.push_back(e);

        if (coin(gen)) storage.add_component(e, Position{float_dist(gen), float_dist(gen)});
        if (coin(gen)) storage.add_component(e, Size{float_dist(gen), float_dist(gen)});
        if (coin(gen)) storage.add_component(e, Color{
            static_cast<uint8_t>(byte_dist(gen)),
            static_cast<uint8_t>(byte_dist(gen)),
            static_cast<uint8_t>(byte_dist(gen)),
            static_cast<uint8_t>(byte_dist(gen))});
        if (coin(gen)) storage.add_component(e, Rotation{float_dist(gen), float_dist(gen)});
        if (coin(gen)) storage.add_component(e, Collider{float_dist(gen), float_dist(gen),
            static_cast<uint8_t>(byte_dist(gen)),
            static_cast<uint8_t>(byte_dist(gen))});
        if (coin(gen)) storage.add_component(e, Lifetime{float_dist(gen)});
        if (coin(gen)) storage.add_component(e, WrapAround{});
    }

    for (Entity e : all_entities) {
        if (coin(gen)) {
            storage.add_component(e, DestroyRequest{});
            marked_entities.push_back(e);
        }
    }

    destroy_marked_entities(em, storage);

    for (Entity e : marked_entities) {
        REQUIRE_FALSE(em.is_alive(e));
        REQUIRE_FALSE(storage.has_component<Position>(e));
        REQUIRE_FALSE(storage.has_component<Size>(e));
        REQUIRE_FALSE(storage.has_component<Color>(e));
        REQUIRE_FALSE(storage.has_component<Input>(e));
        REQUIRE_FALSE(storage.has_component<Velocity>(e));
        REQUIRE_FALSE(storage.has_component<Images>(e));
        REQUIRE_FALSE(storage.has_component<Text>(e));
        REQUIRE_FALSE(storage.has_component<ScreenPosition>(e));
        REQUIRE_FALSE(storage.has_component<Rotation>(e));
        REQUIRE_FALSE(storage.has_component<Collider>(e));
        REQUIRE_FALSE(storage.has_component<Lifetime>(e));
        REQUIRE_FALSE(storage.has_component<WrapAround>(e));
        REQUIRE_FALSE(storage.has_component<DestroyRequest>(e));
    }

    auto remaining = storage.entities_with_component<DestroyRequest>();
    REQUIRE(remaining.empty());
}

TEST_CASE("Property: Destruction pipeline preserves unmarked entities", "[new_component][destruction]") {
    auto seed = GENERATE(take(NUM_OUTER_TESTS, random(0, 1000000)));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> float_dist(-1000.0f, 1000.0f);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<int> coin(0, 1);

    EntityManager em;
    ComponentStorage storage;

    struct SavedComponents {
        Entity entity;
        bool has_position; Position position;
        bool has_size; Size size;
        bool has_color; Color color;
        bool has_rotation; Rotation rotation;
        bool has_collider; Collider collider;
        bool has_lifetime; Lifetime lifetime;
        bool has_wrap;
    };

    std::vector<SavedComponents> unmarked_saved;
    std::vector<Entity> all_entities;

    for (int i = 0; i < NUM_INNER_TESTS; ++i) {
        Entity e = em.create_entity();
        all_entities.push_back(e);

        SavedComponents saved{};
        saved.entity = e;

        if (coin(gen)) {
            Position p{float_dist(gen), float_dist(gen)};
            storage.add_component(e, p);
            saved.has_position = true; saved.position = p;
        }
        if (coin(gen)) {
            Size s{float_dist(gen), float_dist(gen)};
            storage.add_component(e, s);
            saved.has_size = true; saved.size = s;
        }
        if (coin(gen)) {
            Color c{
                static_cast<uint8_t>(byte_dist(gen)),
                static_cast<uint8_t>(byte_dist(gen)),
                static_cast<uint8_t>(byte_dist(gen)),
                static_cast<uint8_t>(byte_dist(gen))};
            storage.add_component(e, c);
            saved.has_color = true; saved.color = c;
        }
        if (coin(gen)) {
            Rotation r{float_dist(gen), float_dist(gen)};
            storage.add_component(e, r);
            saved.has_rotation = true; saved.rotation = r;
        }
        if (coin(gen)) {
            Collider col{float_dist(gen), float_dist(gen),
                static_cast<uint8_t>(byte_dist(gen)),
                static_cast<uint8_t>(byte_dist(gen))};
            storage.add_component(e, col);
            saved.has_collider = true; saved.collider = col;
        }
        if (coin(gen)) {
            Lifetime lt{float_dist(gen)};
            storage.add_component(e, lt);
            saved.has_lifetime = true; saved.lifetime = lt;
        }
        if (coin(gen)) {
            storage.add_component(e, WrapAround{});
            saved.has_wrap = true;
        }

        if (coin(gen)) {
            storage.add_component(e, DestroyRequest{});
        } else {
            unmarked_saved.push_back(saved);
        }
    }

    destroy_marked_entities(em, storage);

    for (const auto& saved : unmarked_saved) {
        Entity e = saved.entity;

        REQUIRE(em.is_alive(e));

        if (saved.has_position) {
            auto p = storage.get_component<Position>(e);
            REQUIRE(p.has_value());
            REQUIRE(p->get().x == saved.position.x);
            REQUIRE(p->get().y == saved.position.y);
        } else {
            REQUIRE_FALSE(storage.has_component<Position>(e));
        }

        if (saved.has_size) {
            auto s = storage.get_component<Size>(e);
            REQUIRE(s.has_value());
            REQUIRE(s->get().width == saved.size.width);
            REQUIRE(s->get().height == saved.size.height);
        } else {
            REQUIRE_FALSE(storage.has_component<Size>(e));
        }

        if (saved.has_color) {
            auto c = storage.get_component<Color>(e);
            REQUIRE(c.has_value());
            REQUIRE(c->get().r == saved.color.r);
            REQUIRE(c->get().g == saved.color.g);
            REQUIRE(c->get().b == saved.color.b);
            REQUIRE(c->get().a == saved.color.a);
        } else {
            REQUIRE_FALSE(storage.has_component<Color>(e));
        }

        if (saved.has_rotation) {
            auto r = storage.get_component<Rotation>(e);
            REQUIRE(r.has_value());
            REQUIRE(r->get().angle == saved.rotation.angle);
            REQUIRE(r->get().angular_velocity == saved.rotation.angular_velocity);
        } else {
            REQUIRE_FALSE(storage.has_component<Rotation>(e));
        }

        if (saved.has_collider) {
            auto col = storage.get_component<Collider>(e);
            REQUIRE(col.has_value());
            REQUIRE(col->get().width == saved.collider.width);
            REQUIRE(col->get().height == saved.collider.height);
            REQUIRE(col->get().layer == saved.collider.layer);
            REQUIRE(col->get().mask == saved.collider.mask);
        } else {
            REQUIRE_FALSE(storage.has_component<Collider>(e));
        }

        if (saved.has_lifetime) {
            auto lt = storage.get_component<Lifetime>(e);
            REQUIRE(lt.has_value());
            REQUIRE(lt->get().remaining == saved.lifetime.remaining);
        } else {
            REQUIRE_FALSE(storage.has_component<Lifetime>(e));
        }

        if (saved.has_wrap) {
            REQUIRE(storage.has_component<WrapAround>(e));
        } else {
            REQUIRE_FALSE(storage.has_component<WrapAround>(e));
        }
    }
}
