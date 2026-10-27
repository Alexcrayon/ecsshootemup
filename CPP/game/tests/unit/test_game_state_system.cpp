/**
 * Unit tests for GameStateSystem (game/game_state_system.hpp)
 *
 * Covers the transitions the state machine owns:
 *   TITLE           -> PLAYING    on a fire press
 *   PLAYING         -> GAME_OVER  when the player's Health reaches 0
 *   PLAYING         -> WIN        when every enemy has been destroyed
 *   GAME_OVER/WIN   -> TITLE      on a fire press, but only after the delay
 *
 * The system reads fire from the player's Input component rather than SDL, which
 * is what makes it testable with no window or event loop.
 */

#include <catch2/catch_test_macros.hpp>
#include "game/game_state_system.hpp"

namespace {

/// A minimal world: one ShipTag player with Input, Health and Position.
struct World {
    ComponentStorage storage;
    Blackboard blackboard;
    Entity player = 1;

    World(int hp = 3) {
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

    Entity add_enemy(Entity id) {
        storage.add_component(id, EnemyTag{});
        storage.add_component(id, Health{1, 1});
        return id;
    }
};

}  // namespace

TEST_CASE("TITLE advances to PLAYING on a fire press", "[game_state][unit]") {
    World w;
    GameStateSystem sys;

    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::TITLE);   // no input yet

    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::PLAYING);
}

TEST_CASE("Holding fire does not re-trigger transitions", "[game_state][unit]") {
    World w;
    GameStateSystem sys;

    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::PLAYING);

    // Player dies while fire is still held; the retry must not fire immediately
    // even after the delay has elapsed, because there was no fresh key press.
    w.storage.get_component<Health>(w.player)->get().current = 0;
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::GAME_OVER);

    w.set_dt(5.0);
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::GAME_OVER);
}

TEST_CASE("PLAYING moves to GAME_OVER when player Health hits zero",
          "[game_state][unit]") {
    World w;
    GameStateSystem sys;

    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    w.set_fire(false);
    REQUIRE(w.state() == game_state::PLAYING);

    w.storage.get_component<Health>(w.player)->get().current = 0;
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::GAME_OVER);
}

TEST_CASE("Retry is refused inside the delay and accepted after it",
          "[game_state][unit]") {
    World w;
    GameStateSystem sys;

    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    w.set_fire(false);
    w.storage.get_component<Health>(w.player)->get().current = 0;
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::GAME_OVER);

    // A press that arrives before RETRY_DELAY_SECONDS is ignored.
    w.set_dt(0.1);
    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::GAME_OVER);

    // Release, wait past the delay, press again.
    w.set_fire(false);
    w.set_dt(1.0);
    sys.update(w.storage, w.blackboard);
    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::TITLE);
}

TEST_CASE("Entering PLAYING restores player Health to max", "[game_state][unit]") {
    World w(3);
    GameStateSystem sys;

    w.storage.get_component<Health>(w.player)->get().current = 1;

    w.set_fire(true);
    sys.update(w.storage, w.blackboard);

    REQUIRE(w.state() == game_state::PLAYING);
    REQUIRE(w.storage.get_component<Health>(w.player)->get().current == 3);
}

TEST_CASE("WIN only when game.all_waves_cleared is set", "[game_state][unit]") {
    // WIN is now owned by EnemySpawnSystem via the game.all_waves_cleared flag,
    // not by GameStateSystem counting enemies.
    World w;
    GameStateSystem sys;

    w.set_fire(true);
    sys.update(w.storage, w.blackboard);
    w.set_fire(false);
    REQUIRE(w.state() == game_state::PLAYING);

    // Flag not set — stays PLAYING even with no enemies present.
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::PLAYING);

    w.blackboard.set<bool>("game.all_waves_cleared", true);
    sys.update(w.storage, w.blackboard);
    REQUIRE(w.state() == game_state::WIN);
}

TEST_CASE("is_playing is true only while PLAYING", "[game_state][unit]") {
    Blackboard bb;

    bb.set<std::string>("game.state", std::string(game_state::TITLE));
    REQUIRE_FALSE(GameStateSystem::is_playing(bb));

    bb.set<std::string>("game.state", std::string(game_state::PLAYING));
    REQUIRE(GameStateSystem::is_playing(bb));

    bb.set<std::string>("game.state", std::string(game_state::GAME_OVER));
    REQUIRE_FALSE(GameStateSystem::is_playing(bb));

    bb.set<std::string>("game.state", std::string(game_state::WIN));
    REQUIRE_FALSE(GameStateSystem::is_playing(bb));

    // Unset key defaults to TITLE, so the simulation stays frozen.
    Blackboard empty;
    REQUIRE_FALSE(GameStateSystem::is_playing(empty));
}
