#include "game/game_state_system.hpp"

namespace {

/// True if any ShipTag entity that has Health has run out of it.
/// A player without a Health component counts as alive.
bool player_is_dead(const ComponentStorage& storage) {
    for (Entity e : storage.entities_with_component<ShipTag>()) {
        auto hp = storage.get_component<Health>(e);
        if (hp.has_value() && hp->get().current <= 0) {
            return true;
        }
    }
    return false;
}

/// Fire is read from the player's Input component rather than SDL directly, so
/// the whole state machine stays testable without a window or event loop.
bool fire_is_down(const ComponentStorage& storage) {
    for (Entity e : storage.entities_with_component<ShipTag>()) {
        auto in = storage.get_component<Input>(e);
        if (in.has_value() && in->get().fire) {
            return true;
        }
    }
    return false;
}

/// Restore the player and clear the arena of projectiles for a fresh run.
void reset_run(ComponentStorage& storage, Blackboard& blackboard) {
    for (Entity e : storage.entities_with_component<ShipTag>()) {
        auto hp = storage.get_component<Health>(e);
        if (hp.has_value()) {
            hp->get().current = hp->get().max;
        }

        // Only move the player if a start position was published; otherwise
        // leave it where it is rather than teleporting it to the origin.
        auto pos = storage.get_component<Position>(e);
        if (pos.has_value() &&
            blackboard.has("player.start_x") && blackboard.has("player.start_y")) {
            pos->get().x = blackboard.get_or<float>("player.start_x", pos->get().x);
            pos->get().y = blackboard.get_or<float>("player.start_y", pos->get().y);
        }

        auto vel = storage.get_component<Velocity>(e);
        if (vel.has_value()) {
            vel->get().dx = 0.0f;
            vel->get().dy = 0.0f;
        }
    }

    // Deferred destruction: the pipeline at the end of the frame does the work.
    for (Entity b : storage.entities_with_component<BulletTag>()) {
        storage.add_component(b, DestroyRequest{});
    }
}

} 

namespace {
// The on-screen banner for each state. Empty during PLAYING so the HUD shows
// nothing mid-fight. A HUD Text entity bound to "hud.banner" renders this.
std::string banner_for(const std::string& state) {
    if (state == game_state::TITLE)     return "Zero Arc\nPress SPACE to start";
    if (state == game_state::WIN)       return "YOU WIN!\nSPACE to play again";
    if (state == game_state::GAME_OVER) return "GAME OVER\nSPACE to play again";
    return "";  // PLAYING
}
}  

std::string GameStateSystem::current(const Blackboard& blackboard) {
    return blackboard.get_or<std::string>("game.state", std::string(game_state::TITLE));
}

bool GameStateSystem::is_playing(const Blackboard& blackboard) {
    return current(blackboard) == game_state::PLAYING;
}

void GameStateSystem::enter_state(const std::string& next,
                                  ComponentStorage& storage,
                                  Blackboard& blackboard) {
    if (next == game_state::PLAYING) {
        reset_run(storage, blackboard);
    }
    blackboard.set<std::string>("game.state", next);
    time_in_state_ = 0.0f;
}

void GameStateSystem::update(ComponentStorage& storage, Blackboard& blackboard) {
    const float dt = static_cast<float>(blackboard.get_or<double>("delta_time", 0.0));
    const std::string state = current(blackboard);

    // check fire button
    // trigger transitions.
    const bool fire_down = fire_is_down(storage);
    const bool fire_pressed = fire_down && !fire_was_down_;
    fire_was_down_ = fire_down;

    time_in_state_ += dt;

    std::string next = state;

    if (state == game_state::TITLE) {
        if (fire_pressed) {
            next = game_state::PLAYING;
        }
    } else if (state == game_state::PLAYING) {
        if (player_is_dead(storage)) {
            next = game_state::GAME_OVER;
        } else if (blackboard.get_or<bool>("game.all_waves_cleared", false)) {
            // EnemySpawnSystem sets this only after the final wave is cleared.
            next = game_state::WIN;
        }
    } else {
        // WIN or GAME_OVER: wait out the delay, then accept a fresh press.
        if (fire_pressed && time_in_state_ >= RETRY_DELAY_SECONDS) {
            next = game_state::TITLE;
        }
    }

    if (next != state) {
        enter_state(next, storage, blackboard);
    }


    blackboard.set<std::string>("hud.banner", banner_for(next));
}
