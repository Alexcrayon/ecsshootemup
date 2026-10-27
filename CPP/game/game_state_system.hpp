#ifndef GAME_STATE_SYSTEM_HPP
#define GAME_STATE_SYSTEM_HPP

#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"
#include <string>

/**
 * Valid values for the "game.state" Blackboard key.
 *
 * Kept as strings (rather than an enum) because a HUD Text entity can bind
 * straight to "game.state" via its blackboard_key and render the value, so the
 * state name doubles as on-screen text with no extra plumbing.
 */
namespace game_state {
inline constexpr const char* TITLE     = "TITLE";
inline constexpr const char* PLAYING   = "PLAYING";
inline constexpr const char* WIN       = "WIN";
inline constexpr const char* GAME_OVER = "GAME_OVER";
}

/**
 * GameStateSystem - owns the game's high-level state machine.
 *
 * This system is the ONLY writer of "game.state". CollisionResponseSystem
 * applies damage; this system decides what that damage means for the run, so
 * there is a single authority for state transitions.
 *
 * Transitions:
 *   TITLE              -> PLAYING    fire pressed
 *   PLAYING            -> GAME_OVER  player Health.current <= 0
 *   PLAYING            -> WIN        every enemy destroyed (see note below)
 *   WIN / GAME_OVER    -> TITLE      fire pressed, after RETRY_DELAY_SECONDS
 *
 * The retry delay matters because the player holds fire almost constantly while
 * dodging — without it, the death screen would be skipped on the same frame it
 * appears. Transitions out of an end state also require a fresh key *press*
 * (rising edge), not merely the key being held.
 *
 * Reads:  Input + Health + Position of ShipTag entities, EnemyTag count,
 *         Blackboard (delta_time, game.state, player.start_x, player.start_y)
 * Writes: Blackboard (game.state), and on entering PLAYING resets the player's
 *         Health/Position/Velocity and clears leftover bullets.
 */
class GameStateSystem {
public:
    /// Seconds an end state must be shown before a retry is accepted.
    static constexpr float RETRY_DELAY_SECONDS = 0.75f;

    void update(ComponentStorage& storage, Blackboard& blackboard);

    /// The single definition of "should the simulation run this frame".
    static bool is_playing(const Blackboard& blackboard);

    /// Current state, defaulting to TITLE when the key has not been set yet.
    static std::string current(const Blackboard& blackboard);

private:
    /// Apply side effects for a transition, then publish the new state.
    void enter_state(const std::string& next,
                     ComponentStorage& storage,
                     Blackboard& blackboard);

    float time_in_state_ = 0.0f;   // seconds since the last transition
    bool  fire_was_down_ = false;  // previous frame's fire, for edge detection
};

#endif // GAME_STATE_SYSTEM_HPP
