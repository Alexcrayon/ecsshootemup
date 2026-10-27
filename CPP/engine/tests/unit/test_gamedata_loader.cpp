/**
 * Unit tests for GameData_Loader
 *
 * These tests verify load_game_data() and its component parsing, Blackboard
 * population, and error handling using temporary JSON files.
 *
 * Testing Framework: Catch2 v3
 * Tags: [gamedata_loader][unit]
 *
 * Validates: Requirements 2.2–2.13, 3.2–3.7, 4.1–4.3, 5.1–5.4, 6.1, 10.1, 10.3–10.6
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "engine/gamedata_loader.hpp"
#include "engine/ecs/entity_manager.hpp"
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

// Helper: write JSON string to a temp file and return the path
static std::string write_temp_json(const std::string& json_str, const std::string& suffix = ".json") {
    auto path = fs::temp_directory_path() / ("test_gamedata_" + std::to_string(std::hash<std::string>{}(json_str)) + suffix);
    std::ofstream out(path);
    out << json_str;
    out.close();
    return path.string();
}

// Helper: remove temp file
static void cleanup(const std::string& path) {
    fs::remove(path);
}

// ---------------------------------------------------------------------------
// 1. Load valid GameData.json creates correct entities
// ---------------------------------------------------------------------------
TEST_CASE("Load valid GameData.json creates correct entities", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "ent_a",
                "components": {
                    "position": {"x": 1.0, "y": 2.0},
                    "size": {"width": 10.0, "height": 20.0}
                }
            },
            {
                "id": "ent_b",
                "components": {
                    "color": {"r": 100, "g": 150, "b": 200, "a": 255}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    REQUIRE(em.active_count() == 2);

    Entity a = bb.get<Entity>("entity.id.ent_a");
    REQUIRE(cs.has_component<Position>(a));
    REQUIRE(cs.has_component<Size>(a));

    auto pos = cs.get_component<Position>(a);
    REQUIRE(pos->get().x == 1.0f);
    REQUIRE(pos->get().y == 2.0f);

    auto sz = cs.get_component<Size>(a);
    REQUIRE(sz->get().width == 10.0f);
    REQUIRE(sz->get().height == 20.0f);

    Entity b = bb.get<Entity>("entity.id.ent_b");
    REQUIRE(cs.has_component<Color>(b));
    auto col = cs.get_component<Color>(b);
    REQUIRE(col->get().r == 100);
    REQUIRE(col->get().g == 150);
    REQUIRE(col->get().b == 200);
    REQUIRE(col->get().a == 255);

    cleanup(path);
}


// ---------------------------------------------------------------------------
// 2. Position component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Position component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "pos_test",
                "components": {
                    "position": {"x": 10.5, "y": 20.3}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.pos_test");
    auto pos = cs.get_component<Position>(e);
    REQUIRE(pos.has_value());
    REQUIRE(pos->get().x == Catch::Approx(10.5f));
    REQUIRE(pos->get().y == Catch::Approx(20.3f));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 3. Size component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Size component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "size_test",
                "components": {
                    "size": {"width": 100.0, "height": 50.0}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.size_test");
    auto sz = cs.get_component<Size>(e);
    REQUIRE(sz.has_value());
    REQUIRE(sz->get().width == Catch::Approx(100.0f));
    REQUIRE(sz->get().height == Catch::Approx(50.0f));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 4. Color component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Color component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "color_test",
                "components": {
                    "color": {"r": 255, "g": 128, "b": 0, "a": 200}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.color_test");
    auto c = cs.get_component<Color>(e);
    REQUIRE(c.has_value());
    REQUIRE(c->get().r == 255);
    REQUIRE(c->get().g == 128);
    REQUIRE(c->get().b == 0);
    REQUIRE(c->get().a == 200);

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 5. Velocity component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Velocity component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "vel_test",
                "components": {
                    "velocity": {"dx": 5.0, "dy": -3.0}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.vel_test");
    auto v = cs.get_component<Velocity>(e);
    REQUIRE(v.has_value());
    REQUIRE(v->get().dx == Catch::Approx(5.0f));
    REQUIRE(v->get().dy == Catch::Approx(-3.0f));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 6. Input component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Input component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "input_test",
                "components": {
                    "input": {}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.input_test");
    REQUIRE(cs.has_component<Input>(e));
    auto inp = cs.get_component<Input>(e);
    REQUIRE(inp->get().up == false);
    REQUIRE(inp->get().down == false);
    REQUIRE(inp->get().left == false);
    REQUIRE(inp->get().right == false);

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 7. Images component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Images component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "img_test",
                "components": {
                    "images": {"names": ["a.png", "b.png"], "active_index": 1}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.img_test");
    auto img = cs.get_component<Images>(e);
    REQUIRE(img.has_value());
    REQUIRE(img->get().filenames.size() == 2);
    REQUIRE(img->get().filenames[0] == "a.png");
    REQUIRE(img->get().filenames[1] == "b.png");
    REQUIRE(img->get().active_index == 1);

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 8. Text component parsing
// ---------------------------------------------------------------------------
TEST_CASE("Text component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "hud_entities": [
            {
                "id": "text_test",
                "components": {
                    "text": {
                        "content": "Hello",
                        "font_name": "test.ttf",
                        "font_size": 16.0,
                        "color": {"r": 255, "g": 255, "b": 255, "a": 255}
                    },
                    "screen_position": {"x": 0.0, "y": 0.0}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.text_test");
    auto txt = cs.get_component<Text>(e);
    REQUIRE(txt.has_value());
    REQUIRE(txt->get().content == "Hello");
    REQUIRE(txt->get().font_name == "test.ttf");
    REQUIRE(txt->get().font_size == Catch::Approx(16.0f));
    REQUIRE(txt->get().color.r == 255);
    REQUIRE(txt->get().color.g == 255);
    REQUIRE(txt->get().color.b == 255);
    REQUIRE(txt->get().color.a == 255);

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 8b. Text blackboard_key (dynamic HUD text bound to a Blackboard string)
// ---------------------------------------------------------------------------
TEST_CASE("Text blackboard_key parses for dynamic HUD text", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "hud_entities": [
            {
                "id": "dyn",
                "components": {
                    "text": {
                        "content": "Calm: 0.0s",
                        "blackboard_key": "game.score_text",
                        "font_name": "test.ttf",
                        "font_size": 24.0,
                        "color": {"r": 255, "g": 255, "b": 255, "a": 255}
                    },
                    "screen_position": {"x": 10.0, "y": 20.0}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.dyn");
    auto txt = cs.get_component<Text>(e);
    REQUIRE(txt.has_value());
    REQUIRE(txt->get().blackboard_key == "game.score_text");
    REQUIRE(txt->get().content == "Calm: 0.0s");  // static fallback preserved

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 9. ScreenPosition component parsing
// ---------------------------------------------------------------------------
TEST_CASE("ScreenPosition component parsing", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "hud_entities": [
            {
                "id": "sp_test",
                "components": {
                    "screen_position": {"x": 10.0, "y": 580.0},
                    "text": {
                        "content": "X",
                        "font_name": "f.ttf",
                        "font_size": 12.0,
                        "color": {"r": 0, "g": 0, "b": 0, "a": 255}
                    }
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.sp_test");
    auto sp = cs.get_component<ScreenPosition>(e);
    REQUIRE(sp.has_value());
    REQUIRE(sp->get().x == Catch::Approx(10.0f));
    REQUIRE(sp->get().y == Catch::Approx(580.0f));

    cleanup(path);
}


// ---------------------------------------------------------------------------
// 10. Entity IDs stored on Blackboard
// ---------------------------------------------------------------------------
TEST_CASE("Entity IDs stored on Blackboard", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {"id": "player", "components": {"position": {"x": 0.0, "y": 0.0}}},
            {"id": "enemy",  "components": {"position": {"x": 1.0, "y": 1.0}}}
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    REQUIRE(bb.has("entity.id.player"));
    REQUIRE(bb.has("entity.id.enemy"));

    Entity player = bb.get<Entity>("entity.id.player");
    Entity enemy  = bb.get<Entity>("entity.id.enemy");
    REQUIRE(player != enemy);
    REQUIRE(em.is_alive(player));
    REQUIRE(em.is_alive(enemy));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 11. Window config on Blackboard
// ---------------------------------------------------------------------------
TEST_CASE("Window config on Blackboard", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"title": "Test", "width": 800, "height": 600}
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    REQUIRE(bb.get<int>("window_width") == 800);
    REQUIRE(bb.get<int>("window_height") == 600);

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 12. Camera config on Blackboard
// ---------------------------------------------------------------------------
TEST_CASE("Camera config on Blackboard", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "camera": {"lookat_x": 1.0, "lookat_y": 2.0, "zoom": 1.5}
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    REQUIRE(bb.get<float>("camera.lookat.x") == Catch::Approx(1.0f));
    REQUIRE(bb.get<float>("camera.lookat.y") == Catch::Approx(2.0f));
    REQUIRE(bb.get<float>("camera.zoom") == Catch::Approx(1.5f));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 13. Camera defaults when absent
// ---------------------------------------------------------------------------
TEST_CASE("Camera defaults when absent", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600}
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    REQUIRE(bb.get<float>("camera.lookat.x") == Catch::Approx(0.0f));
    REQUIRE(bb.get<float>("camera.lookat.y") == Catch::Approx(0.0f));
    REQUIRE(bb.get<float>("camera.zoom") == Catch::Approx(1.0f));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 14. Missing file throws runtime_error
// ---------------------------------------------------------------------------
TEST_CASE("Missing file throws runtime_error", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    REQUIRE_THROWS_AS(
        load_game_data("/nonexistent/path/GameData.json", em, cs, bb),
        std::runtime_error
    );

    try {
        load_game_data("/nonexistent/path/GameData.json", em, cs, bb);
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        REQUIRE(msg.find("/nonexistent/path/GameData.json") != std::string::npos);
    }
}

// ---------------------------------------------------------------------------
// 15. Invalid JSON throws runtime_error
// ---------------------------------------------------------------------------
TEST_CASE("Invalid JSON throws runtime_error", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string bad_json = "{ this is not valid json !!!";
    auto path = write_temp_json(bad_json);

    REQUIRE_THROWS_AS(load_game_data(path, em, cs, bb), std::runtime_error);

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 16. Missing entity ID throws runtime_error
// ---------------------------------------------------------------------------
TEST_CASE("Missing entity ID throws runtime_error", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {"components": {"position": {"x": 0.0, "y": 0.0}}}
        ]
    })";

    auto path = write_temp_json(json_str);

    try {
        load_game_data(path, em, cs, bb);
        FAIL("Expected std::runtime_error");
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        REQUIRE(msg.find("0") != std::string::npos);  // index in message
    }

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 17. Unrecognized component key is skipped
// ---------------------------------------------------------------------------
TEST_CASE("Unrecognized component key is skipped", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "entities": [
            {
                "id": "skip_test",
                "components": {
                    "position": {"x": 5.0, "y": 6.0},
                    "unknown_component": {"foo": "bar"}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.skip_test");
    REQUIRE(cs.has_component<Position>(e));
    REQUIRE_FALSE(cs.has_component<Size>(e));
    REQUIRE_FALSE(cs.has_component<Color>(e));
    REQUIRE_FALSE(cs.has_component<Velocity>(e));
    REQUIRE_FALSE(cs.has_component<Input>(e));
    REQUIRE_FALSE(cs.has_component<Images>(e));
    REQUIRE_FALSE(cs.has_component<Text>(e));
    REQUIRE_FALSE(cs.has_component<ScreenPosition>(e));

    cleanup(path);
}

// ---------------------------------------------------------------------------
// 18. HUD entity creation with Text and ScreenPosition
// ---------------------------------------------------------------------------
TEST_CASE("HUD entity creation with Text and ScreenPosition", "[gamedata_loader][unit]") {
    EntityManager em;
    ComponentStorage cs;
    Blackboard bb;

    std::string json_str = R"({
        "window": {"width": 800, "height": 600},
        "hud_entities": [
            {
                "id": "hud_test",
                "components": {
                    "text": {
                        "content": "HUD",
                        "font_name": "hud.ttf",
                        "font_size": 24.0,
                        "color": {"r": 200, "g": 200, "b": 200, "a": 255}
                    },
                    "screen_position": {"x": 50.0, "y": 100.0}
                }
            }
        ]
    })";

    auto path = write_temp_json(json_str);
    load_game_data(path, em, cs, bb);

    Entity e = bb.get<Entity>("entity.id.hud_test");
    REQUIRE(cs.has_component<Text>(e));
    REQUIRE(cs.has_component<ScreenPosition>(e));

    auto txt = cs.get_component<Text>(e);
    REQUIRE(txt->get().content == "HUD");

    auto sp = cs.get_component<ScreenPosition>(e);
    REQUIRE(sp->get().x == Catch::Approx(50.0f));
    REQUIRE(sp->get().y == Catch::Approx(100.0f));

    cleanup(path);
}
