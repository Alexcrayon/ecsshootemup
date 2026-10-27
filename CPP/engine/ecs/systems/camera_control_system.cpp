/**
 * CameraControlSystem implementation
 *
 * Reads keyboard state each frame and updates camera Blackboard values:
 * - camera.zoom:     modified by +/- keys, clamped to [MIN_ZOOM, MAX_ZOOM]
 * - camera.lookat.x: modified by left/right arrow keys
 * - camera.lookat.y: modified by up/down arrow keys
 *
 * Pan speed is inversely proportional to zoom so that visual panning speed
 * feels consistent regardless of zoom level.
 *
 * Requirements: REQ-1, REQ-2, REQ-3, REQ-4, REQ-5, REQ-6, REQ-7
 */

#include "engine/ecs/systems/camera_control_system.hpp"

#include <algorithm>  // std::clamp
#include <SDL3/SDL.h>

// Camera control constants (REQ-7)
constexpr float ZOOM_SPEED = 1.0f;   // zoom units per second
constexpr float PAN_SPEED  = 300.0f; // world units per second (at zoom 1.0)
constexpr float MIN_ZOOM   = 0.25f;  // minimum zoom (4x viewport)
constexpr float MAX_ZOOM   = 4.0f;   // maximum zoom (1/16 viewport)

void apply_camera_controls(Blackboard& blackboard, const CameraInput& input) {
    // Helper: read a float from Blackboard, handling double→float conversion
    // (tests may store values as double; game code stores as float)
    auto read_float = [&blackboard](const std::string& key, float default_val) -> float {
        if (!blackboard.has(key)) return default_val;
        try { return blackboard.get<float>(key); }
        catch (...) {
            try { return static_cast<float>(blackboard.get<double>(key)); }
            catch (...) { return default_val; }
        }
    };

    // Read current state from Blackboard
    float dt       = read_float("delta_time", 0.0f);
    float zoom     = read_float("camera.zoom", 1.0f);
    float lookat_x = read_float("camera.lookat.x", 0.0f);
    float lookat_y = read_float("camera.lookat.y", 0.0f);

    // Zoom: + key zooms in, - key zooms out (REQ-1, REQ-2)
    if (input.zoom_in)  zoom += ZOOM_SPEED * dt;
    if (input.zoom_out) zoom -= ZOOM_SPEED * dt;

    // Clamp zoom to valid range (REQ-3)
    zoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);

    // Pan: arrow keys, speed inversely proportional to zoom (REQ-4, REQ-5)
    float effective_pan = PAN_SPEED * dt / zoom;
    if (input.pan_left)  lookat_x -= effective_pan;
    if (input.pan_right) lookat_x += effective_pan;
    if (input.pan_up)    lookat_y += effective_pan;
    if (input.pan_down)  lookat_y -= effective_pan;

    // Write updated values back to Blackboard
    blackboard.set("camera.zoom", zoom);
    blackboard.set("camera.lookat.x", lookat_x);
    blackboard.set("camera.lookat.y", lookat_y);
}

void CameraControlSystem::update(Blackboard& blackboard) {
    const bool* keys = SDL_GetKeyboardState(nullptr);

    CameraInput input;
    input.zoom_in   = keys[SDL_SCANCODE_EQUALS];
    input.zoom_out  = keys[SDL_SCANCODE_MINUS];
    input.pan_left  = keys[SDL_SCANCODE_A];
    input.pan_right = keys[SDL_SCANCODE_D];
    input.pan_up    = keys[SDL_SCANCODE_W];
    input.pan_down  = keys[SDL_SCANCODE_S];

    apply_camera_controls(blackboard, input);
}
