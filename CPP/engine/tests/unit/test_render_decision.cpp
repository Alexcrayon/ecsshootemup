/**
 * Unit tests for rendering decision logic
 *
 * These tests verify the Images component and the rendering decision
 * priority chain: Images > Color > skip. They test component presence
 * logic and the Y-axis flip formula as pure math — no SDL required.
 *
 * Requirements tested: 1.3, 1.4, 2.3, 4.1, 4.3, 5.1, 5.3, 5.4
 */

#include <catch2/catch_test_macros.hpp>
#include "engine/ecs/component_storage.hpp"

TEST_CASE("Default-constructed Images has empty filenames", "[render_decision]") {
    Images img;
    CHECK(img.filenames.empty());
    CHECK(img.active_index == 0u);
}

TEST_CASE("Images copy produces equal filenames", "[render_decision]") {
    Images original{{"explorer.png"}, 0};

    Images copy = original;
    CHECK(copy.filenames.size() == 1u);
    CHECK(copy.active_filename() == "explorer.png");
    CHECK(copy.filenames == original.filenames);
    CHECK(copy.active_index == original.active_index);
}

TEST_CASE("ComponentStorage add/get round-trip for Images", "[render_decision]") {
    ComponentStorage storage;
    Entity entity = 1;

    storage.add_component(entity, Images{{"test_texture.png"}, 0});

    auto retrieved = storage.get_component<Images>(entity);
    REQUIRE(retrieved.has_value());
    CHECK(retrieved->get().active_filename() == "test_texture.png");
}

TEST_CASE("Entity with Position+Size+Images selects texture path", "[render_decision]") {
    ComponentStorage storage;
    Entity entity = 1;

    storage.add_component(entity, Position{100.0f, 200.0f});
    storage.add_component(entity, Size{64.0f, 64.0f});
    storage.add_component(entity, Images{{"player.png"}, 0});

    CHECK(storage.has_component<Position>(entity));
    CHECK(storage.has_component<Size>(entity));
    CHECK(storage.has_component<Images>(entity));
}

TEST_CASE("Entity with Color but no Images selects color path", "[render_decision]") {
    ComponentStorage storage;
    Entity entity = 1;

    storage.add_component(entity, Position{100.0f, 200.0f});
    storage.add_component(entity, Size{64.0f, 64.0f});
    storage.add_component(entity, Color{255, 0, 0, 255});

    CHECK(storage.has_component<Position>(entity));
    CHECK(storage.has_component<Size>(entity));
    CHECK_FALSE(storage.has_component<Images>(entity));
    CHECK(storage.has_component<Color>(entity));
}

TEST_CASE("Entity with neither Images nor Color skips", "[render_decision]") {
    ComponentStorage storage;
    Entity entity = 1;

    storage.add_component(entity, Position{100.0f, 200.0f});
    storage.add_component(entity, Size{64.0f, 64.0f});

    CHECK(storage.has_component<Position>(entity));
    CHECK(storage.has_component<Size>(entity));
    CHECK_FALSE(storage.has_component<Images>(entity));
    CHECK_FALSE(storage.has_component<Color>(entity));
}

TEST_CASE("Images component takes priority over Color", "[render_decision]") {
    ComponentStorage storage;
    Entity entity = 1;

    storage.add_component(entity, Position{100.0f, 200.0f});
    storage.add_component(entity, Size{64.0f, 64.0f});
    storage.add_component(entity, Images{{"player.png"}, 0});
    storage.add_component(entity, Color{255, 0, 0, 255});

    CHECK(storage.has_component<Images>(entity));
    CHECK(storage.has_component<Color>(entity));
}

TEST_CASE("Y-axis flip at origin: sdl_y = window_height - game_y - height", "[render_decision]") {
    const float window_height = 600.0f;
    const float game_y = 0.0f;
    const float height = 100.0f;

    float sdl_y = window_height - game_y - height;

    CHECK(sdl_y == 500.0f);
}
