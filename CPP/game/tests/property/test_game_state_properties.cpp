/**
 * Property-based tests for GameStateSystem (game/game_state_system.hpp)
 *
 * Properties verified:
 *   Property 1: game.state is always one of the four declared values, for any
 *               sequence of frame times and fire inputs.
 *   Property 2: A dead player in PLAYING always reaches GAME_OVER in one update,
 *               regardless of frame time or input.
 *   Property 3: The simulation gate is exactly "state == PLAYING" — never true
 *               in an end state, whatever the frame time.
 *   Property 4: Entering PLAYING always restores Health to max, from any
 *               starting current value.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "game/game_state_system.hpp"

// Configurable test iteration counts (MANDATORY — workspace policy)
constexpr int NUM_OUTER_TESTS = 10;  // Number of different test scenarios
constexpr int NUM_INNER_TESTS = 5;   // Number of iterations per scenario

namespace {

struct World {
    ComponentStorage storage;
    Blackboard blackboard;
    Entity player = 1;

    explicit World(int hp = 3) {
        storage.add_component(player, ShipTag{});
        storage.add_component(player, Input{});
        storage.add_component(player, Health{hp, hp});
        storage.add_component(player, Position{0.0f, 0.0f});
        blackboard.set<double>("delta_time", 1.0 / 60.0);
        blackboard.set<std::string>("game.state", std::string(game_state::TITLE));
    }

    void set_fire(bool down) {
        storage.get_component<Input>(player)->get().fire = down;
    }
    void set_dt(double dt) { blackboard.set<double>("delta_time", dt); }
    std::string state() const { return GameStateSystem::current(blackboard); }
};

bool is_valid_state(const std::string& s) {
    return s == game_state::TITLE || s == game_state::PLAYING ||
           s == game_state::WIN   || s == game_state::GAME_OVER;
}

}  // namespace

// ---------------------------------------------------------------------------
// Property 1: the state is never anything other than the four declared values.
// ---------------------------------------------------------------------------
TEST_CASE("State always remains one of the four declared values",
          "[game_state][property]") {
    SECTION("arbitrary sequences of frame time and fire input") {
        auto seed  = GENERATE(take(NUM_OUTER_TESTS, random(0, 100000)));
        auto steps = GENERATE(take(NUM_INNER_TESTS, random(1, 40)));

        World w;
        GameStateSystem sys;

        // A cheap deterministic PRNG so a failure is reproducible from the seed.
        unsigned state = static_cast<unsigned>(seed) | 1u;
        auto next_bit = [&state]() {
            state = state * 1664525u + 1013904223u;
            return ((state >> 16) & 1u) != 0u;
        };

        for (int i = 0; i < steps; ++i) {
            w.set_fire(next_bit());
            w.set_dt(next_bit() ? 1.0 / 30.0 : 1.0 / 120.0);

            // Occasionally kill the player to drive it into an end state.
            if (next_bit()) {
                w.storage.get_component<Health>(w.player)->get().current = 0;
            }

            sys.update(w.storage, w.blackboard);
            REQUIRE(is_valid_state(w.state()));
        }
    }
}

// ---------------------------------------------------------------------------
// Property 2: from PLAYING, zero Health always yields GAME_OVER in one update.
// ---------------------------------------------------------------------------
TEST_CASE("A dead player always reaches GAME_OVER in one update",
          "[game_state][property]") {
    SECTION("independent of frame time and fire state") {
        auto dt_ms     = GENERATE(take(NUM_OUTER_TESTS, random(1, 500)));
        auto fire_flag = GENERATE(take(NUM_INNER_TESTS, random(0, 1)));

        World w;
        GameStateSystem sys;

        // Get into PLAYING first.
        w.set_fire(true);
        sys.update(w.storage, w.blackboard);
        w.set_fire(false);
        REQUIRE(w.state() == game_state::PLAYING);

        w.storage.get_component<Health>(w.player)->get().current = 0;
        w.set_dt(static_cast<double>(dt_ms) / 1000.0);
        w.set_fire(fire_flag != 0);

        sys.update(w.storage, w.blackboard);
        REQUIRE(w.state() == game_state::GAME_OVER);
    }
}

// ---------------------------------------------------------------------------
// Property 3: the simulation gate is exactly "state == PLAYING".
// ---------------------------------------------------------------------------
TEST_CASE("is_playing is never true outside PLAYING", "[game_state][property]") {
    SECTION("for every declared state and any frame time") {
        auto which  = GENERATE(take(NUM_OUTER_TESTS, random(0, 3)));
        auto dt_ms  = GENERATE(take(NUM_INNER_TESTS, random(1, 500)));

        const char* states[] = { game_state::TITLE, game_state::PLAYING,
                                 game_state::WIN,   game_state::GAME_OVER };
        const char* chosen = states[which % 4];

        Blackboard bb;
        bb.set<double>("delta_time", static_cast<double>(dt_ms) / 1000.0);
        bb.set<std::string>("game.state", std::string(chosen));

        const bool expected = (std::string(chosen) == game_state::PLAYING);
        REQUIRE(GameStateSystem::is_playing(bb) == expected);
    }
}

// ---------------------------------------------------------------------------
// Property 4: entering PLAYING always restores Health to max.
// ---------------------------------------------------------------------------
TEST_CASE("Entering PLAYING always restores Health to max",
          "[game_state][property]") {
    SECTION("from any starting current value and any max") {
        auto max_hp  = GENERATE(take(NUM_OUTER_TESTS, random(1, 20)));
        auto current = GENERATE(take(NUM_INNER_TESTS, random(-5, 20)));

        World w(max_hp);
        GameStateSystem sys;
        w.storage.get_component<Health>(w.player)->get().current = current;

        w.set_fire(true);
        sys.update(w.storage, w.blackboard);

        REQUIRE(w.state() == game_state::PLAYING);
        REQUIRE(w.storage.get_component<Health>(w.player)->get().current == max_hp);
    }
}
