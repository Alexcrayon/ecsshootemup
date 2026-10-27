/**
 * Class-070: Duck Pond — Lua Scripting Shell
 *
 * A minimal game shell with the full engine infrastructure intact but no
 * game-specific systems. This serves as the starting point for the Duck
 * Pond game, which will be driven by Lua scripts.
 *
 * World space uses a centered origin: (0,0) is the center of the world,
 * positive X rightward, positive Y upward (Cartesian convention).
 *
 * System execution order:
 *   InputSystem → RotationSystem → MovementSystem → WrapSystem →
 *   CollisionSystem → LifetimeSystem → ScriptSystem → destroy_marked_entities →
 *   CameraSystem → RenderSystem → HUDSystem → DebugHUDSystem
 *
 * Controls:
 *   H                 — Toggle HUD visibility
 *   F1                — Toggle debug pause
 *   F2                — Single step (while paused)
 *   F10               — Toggle debug HUD
 *   ESC / Close       — Exit
 *
 * Requirements: 8.1–8.7
 */

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <ctime>

#include "engine/ecs/entity_manager.hpp"
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/blackboard.hpp"
#include "engine/timer.hpp"
#include "engine/resource_manager.hpp"
#include "engine/project_paths.hpp"
// NEW (Class-110): audio ported from the Class-070 audio option.
#include <SDL3_mixer/SDL_mixer.h>
#include "engine/audio_manager.hpp"
#include "engine/ecs/systems/audio_system.hpp"
#include "engine/ecs/systems/render_system.hpp"
#include "engine/ecs/systems/input_system.hpp"
#include "engine/ecs/systems/movement_system.hpp"
#include "engine/ecs/systems/hud_system.hpp"
#include "engine/ecs/systems/camera_system.hpp"
#include "engine/ecs/systems/rotation_system.hpp"
#include "engine/ecs/systems/collision_system.hpp"
#include "engine/ecs/systems/brute_force_strategy.hpp"
#include "engine/ecs/systems/uniform_grid_strategy.hpp"
#include "engine/ecs/systems/wrap_system.hpp"
#include "engine/ecs/systems/lifetime_system.hpp"
// NEW (Class-110): player movement was never wired into the Duck Pond base;
// needed so the bullet-hell player can move to dodge. Engine system, Class-020.
#include "engine/ecs/systems/player_control_system.hpp"
// NEW (Class-110): game-specific systems ported from Class-050 (player fire +
// hit-damage response).
#include "game/bullet_spawn_system.hpp"
#include "game/collision_response_system.hpp"
// NEW (Class-110): owns the TITLE -> PLAYING -> WIN/GAME_OVER state machine and
// is the single writer of the "game.state" Blackboard key.
#include "game/game_state_system.hpp"
// NEW (Class-110): spawns enemies from JSON wave definitions.
#include "game/enemy_spawn_system.hpp"
// NEW (Class-110): loops multi-frame Images sprites (e.g. the player ship).
#include "game/sprite_anim_system.hpp"
// NEW (Class-110): brief hit flash on collision.
#include "game/flash_system.hpp"
#include "engine/ecs/destruction.hpp"
#include "cli_parser.hpp"
#include "script_loader.hpp"
#include "engine/ecs/systems/screenshot_system.hpp"
#include "debug_state.hpp"
#include "engine/ecs/systems/debug_hud_system.hpp"
#include "engine/lua_manager.hpp"
#include "engine/ecs/systems/script_system.hpp"
#include "engine/gamedata_loader.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>

// Debug infrastructure for frame dump/trace
#include "engine/ecs/debug/json_serializer.hpp"
#include "engine/ecs/debug/debug_types.hpp"

/**
 * Custom deleter for SDL_Window
 * Ensures proper cleanup of SDL resources using RAII
 */
struct SDL_WindowDeleter {
    void operator()(SDL_Window* window) const {
        if (window) {
            SDL_DestroyWindow(window);
        }
    }
};

/**
 * Custom deleter for SDL_Renderer
 * Ensures proper cleanup of SDL resources using RAII
 */
struct SDL_RendererDeleter {
    void operator()(SDL_Renderer* renderer) const {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
    }
};

// Type aliases for smart pointers with custom deleters
using SDL_WindowPtr = std::unique_ptr<SDL_Window, SDL_WindowDeleter>;
using SDL_RendererPtr = std::unique_ptr<SDL_Renderer, SDL_RendererDeleter>;

/**
 * Concrete implementation of IBlackboardAccessor for the Blackboard class.
 * Allows JSONSerializer to access blackboard values for frame dump/trace.
 * Read-only interface — the serializer never writes to the blackboard.
 */
class BlackboardAccessor : public engine::debug::IBlackboardAccessor {
public:
    explicit BlackboardAccessor(Blackboard& blackboard) : blackboard_(blackboard) {}

    engine::debug::Value get_value(const std::string& key) const override {
        engine::debug::Value result;

        // Try int
        try {
            int val = blackboard_.get<int>(key);
            result.type = engine::debug::Value::Type::Int;
            result.data = static_cast<int64_t>(val);
            return result;
        } catch (...) {}

        // Try float
        try {
            float val = blackboard_.get<float>(key);
            result.type = engine::debug::Value::Type::Float;
            result.data = static_cast<double>(val);
            return result;
        } catch (...) {}

        // Try double
        try {
            double val = blackboard_.get<double>(key);
            result.type = engine::debug::Value::Type::Float;
            result.data = val;
            return result;
        } catch (...) {}

        // Try bool
        try {
            bool val = blackboard_.get<bool>(key);
            result.type = engine::debug::Value::Type::Bool;
            result.data = val;
            return result;
        } catch (...) {}

        // Try string
        try {
            std::string val = blackboard_.get<std::string>(key);
            result.type = engine::debug::Value::Type::String;
            result.data = val;
            return result;
        } catch (...) {}

        // Try uint64_t (for frame_count)
        try {
            uint64_t val = blackboard_.get<uint64_t>(key);
            result.type = engine::debug::Value::Type::Int;
            result.data = static_cast<int64_t>(val);
            return result;
        } catch (...) {}

        throw std::runtime_error("Blackboard key '" + key + "' not found or unsupported type");
    }

    std::vector<std::string> get_all_keys() const override {
        return blackboard_.get_all_keys();
    }

private:
    Blackboard& blackboard_;
};

// --- Helper functions to reduce Value construction boilerplate ---
inline std::unique_ptr<engine::debug::Value> make_float_value(double v) {
    auto val = std::make_unique<engine::debug::Value>();
    val->type = engine::debug::Value::Type::Float;
    val->data = v;
    return val;
}
inline std::unique_ptr<engine::debug::Value> make_int_value(int64_t v) {
    auto val = std::make_unique<engine::debug::Value>();
    val->type = engine::debug::Value::Type::Int;
    val->data = v;
    return val;
}
inline std::unique_ptr<engine::debug::Value> make_bool_value(bool v) {
    auto val = std::make_unique<engine::debug::Value>();
    val->type = engine::debug::Value::Type::Bool;
    val->data = v;
    return val;
}
inline std::unique_ptr<engine::debug::Value> make_string_value(const std::string& v) {
    auto val = std::make_unique<engine::debug::Value>();
    val->type = engine::debug::Value::Type::String;
    val->data = v;
    return val;
}

// --- ComponentBridge adapter: type-erased component access ---
template<typename T>
class ComponentBridgeAdapter : public engine::debug::IComponentBridge {
public:
    std::optional<std::any> get_as_any(Entity entity, ComponentStorage& cs) const override {
        auto opt = cs.get_component<T>(entity);
        if (!opt.has_value()) return std::nullopt;
        return std::any(opt->get());
    }
    bool has(Entity entity, const ComponentStorage& cs) const override {
        return cs.has_component<T>(entity);
    }
};

// --- PropertyAccessor adapter: component registry ---
class PropertyAccessorAdapter : public engine::debug::IPropertyAccessor {
public:
    template<typename T>
    void register_component(const std::string& name) {
        bridges_.push_back({name, std::make_unique<ComponentBridgeAdapter<T>>()});
    }
    std::vector<std::pair<std::string, const engine::debug::IComponentBridge*>>
    registered_components() const override {
        std::vector<std::pair<std::string, const engine::debug::IComponentBridge*>> result;
        for (const auto& [name, bridge] : bridges_) {
            result.push_back({name, bridge.get()});
        }
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        return result;
    }
private:
    std::vector<std::pair<std::string, std::unique_ptr<engine::debug::IComponentBridge>>> bridges_;
};

// --- EntityMapper adapter ---
class EntityMapperAdapter : public engine::debug::IEntityMapper {
public:
    void map(int logical_id, Entity actual_id) {
        mapping_[logical_id] = actual_id;
    }
    std::vector<int> get_logical_ids() const override {
        std::vector<int> ids;
        for (const auto& [lid, _] : mapping_) ids.push_back(lid);
        return ids;
    }
    Entity get_actual_id(int logical_id) const override {
        return mapping_.at(logical_id);
    }
private:
    std::unordered_map<int, Entity> mapping_;
};

// --- TypeIntrospector adapter: type metadata and value conversion ---
class TypeIntrospectorAdapter : public engine::debug::ITypeIntrospector {
public:
    using ValueConverter = std::function<engine::debug::Value(const std::any&)>;

    void register_struct(const std::string& name,
                         std::vector<engine::debug::FieldInfo> fields,
                         ValueConverter converter) {
        engine::debug::TypeInfo info;
        info.name = name;
        info.kind = engine::debug::TypeInfo::Kind::Struct;
        info.fields = std::move(fields);
        types_[name] = std::move(info);
        converters_[name] = std::move(converter);
    }

    bool has_type(const std::string& type_name) const override {
        return types_.count(type_name) > 0;
    }
    const engine::debug::TypeInfo& get_type(const std::string& type_name) const override {
        return types_.at(type_name);
    }
    engine::debug::Value cpp_to_value(const std::any& cpp_value,
                                       const std::string& type_name) const override {
        return converters_.at(type_name)(cpp_value);
    }
private:
    std::unordered_map<std::string, engine::debug::TypeInfo> types_;
    std::unordered_map<std::string, ValueConverter> converters_;
};

// --- Local timestamp helper (replaces JSONSerializer::current_timestamp() which is private) ---
static std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H-%M-%S", &tm_buf);
    return std::string(buf);
}

int main(int argc, char* argv[]) {
    // Parse command-line debug options before any SDL initialization
    auto opts = parse_command_line(argc, argv);
    if (opts.help_requested) return 0;
    if (opts.parse_error) return 1;

    // Load script file if specified (overrides all other options)
    if (!opts.script_file.empty()) {
        try {
            std::string script_path = opts.script_file;
            opts = load_script(script_path);
            opts.script_file = script_path;
            std::cout << "Loaded script: " << script_path << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    // Initialize SDL3 video + audio subsystems
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL3_ttf for font rendering (required by HUDSystem)
    if (!TTF_Init()) {
        std::cerr << "Failed to initialize SDL_ttf: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }


    MIX_Mixer* mixer = nullptr;
    if (MIX_Init()) {
        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (!mixer) {
            std::cerr << "[Audio] MIX_CreateMixerDevice failed: " << SDL_GetError()
                      << " — running without audio" << std::endl;
        }
    } else {
        std::cerr << "[Audio] MIX_Init failed: " << SDL_GetError()
                  << " — running without audio" << std::endl;
    }

    // Create window 
    SDL_WindowPtr window(
        SDL_CreateWindow(
            "Zero Arc",
            540, 960,
            SDL_WINDOW_RESIZABLE
        )
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer with hardware acceleration
    SDL_RendererPtr renderer(
        SDL_CreateRenderer(window.get(), nullptr)
    );

    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // === ECS Setup ===

    EntityManager entity_manager;
    ComponentStorage component_storage;
    Blackboard blackboard;

    // Initialize Blackboard with game state
    blackboard.set("hud_visible", true);

    // Load entities from GameData.json
    std::string gamedata_path = project_paths::assets_dir() + "/GameData.json";
    load_game_data(gamedata_path, entity_manager, component_storage, blackboard);

    // NEW (Class-110): attach the player's identity tag in code.
    //
    // ShipTag is a structural marker with no tunable value, so it deliberately
    // does NOT appear in GameData.json — that file stays a pure design surface
    // (stats, sizes, sprites) and a designer never has to know tags exist.
    //
    // The loader publishes every JSON entity's id as "entity.id.<id>", which
    // gives a clean handle to the player without inventing a JSON encoding for
    // empty tag components. Enemies/bullets differ: they are created repeatedly
    // at runtime, so their tags get attached by their spawner instead.
    try {
        Entity player_entity = blackboard.get<Entity>("entity.id.player");
        component_storage.add_component(player_entity, ShipTag{});
    } catch (const std::exception&) {
        std::cerr << "Warning: no 'player' entity found in GameData.json — "
                  << "player-specific logic (damage, fire) will be inactive."
                  << std::endl;
    }


    // set the game state to title when loaded
    try {
        Entity p = blackboard.get<Entity>("entity.id.player");
        auto start_pos = component_storage.get_component<Position>(p);
        if (start_pos.has_value()) {
            blackboard.set<float>("player.start_x", start_pos->get().x);
            blackboard.set<float>("player.start_y", start_pos->get().y);
        }
    }
    catch (const std::exception&) {
        // no player entity — GameStateSystem simply won't reposition anything
    }
    blackboard.set<std::string>("game.state", game_state::TITLE);


    // Create ResourceManager (loads textures/fonts from assets directory).
    // Held by unique_ptr so it can be destroyed explicitly at shutdown
    // BEFORE TTF_Quit()/SDL_Quit() — its destructor calls SDL_DestroyTexture and
    // TTF_CloseFont, which must run while SDL/TTF are still initialized.
    auto resource_manager_ptr =
        std::make_unique<ResourceManager>(renderer.get(), project_paths::assets_dir());
    ResourceManager& resource_manager = *resource_manager_ptr;


    auto audio_manager_ptr = std::make_unique<AudioManager>(mixer, project_paths::assets_dir());
    auto audio_system_ptr = std::make_unique<AudioSystem>(mixer, *audio_manager_ptr);
    AudioSystem& audio_system = *audio_system_ptr;


    audio_system.play_sound("silver_bullet.mp3", 0.1f, /*looping=*/true);

    // Create Timer with target FPS (CLI override or default 60)
    Timer timer(opts.fps > 0 ? static_cast<double>(opts.fps) : 60.0);
    if (opts.seed.has_value()) {
        timer.set_deterministic(true);
    }
    timer.update_blackboard(blackboard);

    // Create engine systems
    InputSystem input_system;
    MovementSystem movement_system;
    RenderSystem render_system(renderer.get(), resource_manager);
    render_system.set_background_texture("");
    CameraSystem camera_system;
    HUDSystem hud_system(renderer.get(), resource_manager, 540, 960);

    // Create engine simulation systems
    RotationSystem rotation_system;
    

    //switch to uniform grid strategy for large number of bullet
    UniformGridStrategy grid_strategy(64, -270.0f, -480.0f, 540.0f, 960.0f);
    CollisionSystem collision_system(grid_strategy);
    WrapSystem wrap_system;
    LifetimeSystem lifetime_system;

    // Lua scripting
    LuaManager lua_manager;
    ScriptSystem script_system(lua_manager);

    // NEW (Class-110): game-specific systems for the bullet-hell.
    // PlayerControlSystem converts arrow/WASD Input into Velocity; move speed is
    // data-driven via GameData.json "player.move_speed" (falls back to 200).
    PlayerControlSystem player_control_system(
        blackboard.get_or<float>("player.move_speed", 300.0f));
    // BulletSpawnSystem: reads Input.fire on the player and spawns capped,
    // cooldown-gated bullets (config under the "bullet.*" Blackboard keys).
    BulletSpawnSystem bullet_spawn_system;
    // CollisionResponseSystem: subtracts Health on hits and spends bullets
    // (rewritten from the Class-050 version). It does NOT touch game.state.
    CollisionResponseSystem collision_response_system;
    // GameStateSystem: the only writer of game.state.
    GameStateSystem game_state_system;
    // EnemySpawnSystem: waves loaded from the same GameData.json.
    EnemySpawnSystem enemy_spawn_system;
    enemy_spawn_system.configure(load_enemy_config(gamedata_path));
    // SpriteAnimSystem: loops any Images sprite with 2+ frames.
    SpriteAnimSystem sprite_anim_system;
    // FlashSystem: drives the brief hit flash on damaged entities.
    FlashSystem flash_system;

    // Debug state
    bool debug_paused = false;
    bool debug_hud_visible = false;
    bool step_requested = false;

    // Interactive dump/trace capture (J/T keys). One-shot: set when the key is
    // pressed or injected, folded into is_dump_frame/is_trace_frame, then cleared
    // after the step completes (alongside step_requested).
    bool force_dump_this_step = false;   // J: dump this frame (before -> advance -> after)
    bool force_trace_this_step = false;  // T: full per-system trace of this frame

    // Apply --paused CLI option
    if (opts.paused) {
        debug_paused = true;
        debug_hud_visible = true;
    }

    // Create log directory for this run
    std::string log_dir = create_log_directory(opts.clear_logs);
    std::cout << "Log directory: " << log_dir << std::endl;

    // Create screenshot system
    ScreenshotSystem screenshot_system(renderer.get(), log_dir);

    // Edge detection state
    bool f1_was_pressed = false;
    bool f2_was_pressed = false;
    bool f10_was_pressed = false;
    bool j_was_pressed = false;
    bool t_was_pressed = false;

    // Key injection state: track whether arrow keys were injected last frame
    bool injected_arrow_last_frame = false;

    // Create DebugHUDSystem (overlay alpha from GameData.json via Blackboard)
    uint8_t debug_overlay_alpha = static_cast<uint8_t>(
        blackboard.get_or<int>("debug.overlay_alpha", 128));
    DebugHUDSystem debug_hud_system(renderer.get(), resource_manager, 540, 960,
                                    debug_overlay_alpha);

    // === Debug infrastructure for frame dump/trace ===
    std::unique_ptr<PropertyAccessorAdapter> prop_accessor;
    std::unique_ptr<TypeIntrospectorAdapter> introspector;
    std::unique_ptr<EntityMapperAdapter> ent_mapper;
    std::unique_ptr<BlackboardAccessor> bb_accessor;
    std::unique_ptr<engine::debug::JSONSerializer> json_serializer;

    bool dump_trace_enabled = !opts.dump_frames.empty() || !opts.trace_frames.empty();

    // The serializer is needed for frame-number-driven --dump/--trace AND for the
    // interactive J/T keys. dump_trace_enabled keeps its meaning: it gates ONLY the
    // frame-number auto-dump matching below. The debug stack is built whenever either
    // is in play. (J/T serialize nothing until pressed.)
    bool serializer_needed = dump_trace_enabled || opts.debug_keys;

    if (serializer_needed) {
        prop_accessor = std::make_unique<PropertyAccessorAdapter>();
        introspector = std::make_unique<TypeIntrospectorAdapter>();
        ent_mapper = std::make_unique<EntityMapperAdapter>();

        // Register component types with adapter classes
        prop_accessor->register_component<Position>("Position");
        introspector->register_struct("Position",
            {{"x", "float"}, {"y", "float"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& p = std::any_cast<const Position&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["x"] = make_float_value(static_cast<double>(p.x));
                obj["y"] = make_float_value(static_cast<double>(p.y));
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<Size>("Size");
        introspector->register_struct("Size",
            {{"height", "float"}, {"width", "float"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& s = std::any_cast<const Size&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["width"] = make_float_value(static_cast<double>(s.width));
                obj["height"] = make_float_value(static_cast<double>(s.height));
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<Color>("Color");
        introspector->register_struct("Color",
            {{"a", "uint8_t"}, {"b", "uint8_t"}, {"g", "uint8_t"}, {"r", "uint8_t"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& c = std::any_cast<const Color&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["r"] = make_int_value(static_cast<int64_t>(c.r));
                obj["g"] = make_int_value(static_cast<int64_t>(c.g));
                obj["b"] = make_int_value(static_cast<int64_t>(c.b));
                obj["a"] = make_int_value(static_cast<int64_t>(c.a));
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<Velocity>("Velocity");
        introspector->register_struct("Velocity",
            {{"dx", "float"}, {"dy", "float"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& vel = std::any_cast<const Velocity&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["dx"] = make_float_value(static_cast<double>(vel.dx));
                obj["dy"] = make_float_value(static_cast<double>(vel.dy));
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<Input>("Input");
        introspector->register_struct("Input",
            {{"down", "bool"}, {"fire", "bool"}, {"left", "bool"}, {"right", "bool"}, {"up", "bool"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& inp = std::any_cast<const Input&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["up"] = make_bool_value(inp.up);
                obj["down"] = make_bool_value(inp.down);
                obj["left"] = make_bool_value(inp.left);
                obj["right"] = make_bool_value(inp.right);
                obj["fire"] = make_bool_value(inp.fire);
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<Images>("Images");
        introspector->register_struct("Images",
            {{"active_index", "uint32_t"}, {"filenames", "vector<std::string>"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& img = std::any_cast<const Images&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["active_index"] = make_int_value(static_cast<int64_t>(img.active_index));
                // filenames as array of strings
                std::vector<std::unique_ptr<engine::debug::Value>> arr;
                for (const auto& fn : img.filenames) {
                    arr.push_back(make_string_value(fn));
                }
                auto arr_val = std::make_unique<engine::debug::Value>();
                arr_val->type = engine::debug::Value::Type::Array;
                arr_val->data = std::move(arr);
                obj["filenames"] = std::move(arr_val);
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<ScreenPosition>("ScreenPosition");
        introspector->register_struct("ScreenPosition",
            {{"x", "float"}, {"y", "float"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& sp = std::any_cast<const ScreenPosition&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["x"] = make_float_value(static_cast<double>(sp.x));
                obj["y"] = make_float_value(static_cast<double>(sp.y));
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<Rotation>("Rotation");
        introspector->register_struct("Rotation",
            {{"angle", "float"}, {"angular_velocity", "float"}},
            [](const std::any& v) -> engine::debug::Value {
                const auto& r = std::any_cast<const Rotation&>(v);
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                obj["angle"] = make_float_value(static_cast<double>(r.angle));
                obj["angular_velocity"] = make_float_value(static_cast<double>(r.angular_velocity));
                val.data = std::move(obj);
                return val;
            });

        prop_accessor->register_component<WrapAround>("WrapAround");
        introspector->register_struct("WrapAround",
            {},
            [](const std::any& /*v*/) -> engine::debug::Value {
                engine::debug::Value val;
                val.type = engine::debug::Value::Type::Object;
                std::unordered_map<std::string, std::unique_ptr<engine::debug::Value>> obj;
                val.data = std::move(obj);
                return val;
            });

        // Create BlackboardAccessor and JSONSerializer
        bb_accessor = std::make_unique<BlackboardAccessor>(blackboard);
        json_serializer = std::make_unique<engine::debug::JSONSerializer>(
            *prop_accessor, *introspector, *ent_mapper, bb_accessor.get());

        // Populate entity mapper from blackboard entity.id.* keys (dynamic mapping)
        {
            int logical = 0;
            auto all_keys = blackboard.get_all_keys();
            std::sort(all_keys.begin(), all_keys.end());
            for (const auto& key : all_keys) {
                if (key.rfind("entity.id.", 0) == 0) {
                    try {
                        Entity actual = blackboard.get<Entity>(key);
                        ent_mapper->map(logical, actual);
                        ++logical;
                    } catch (...) {}
                }
            }
        }
    }

    std::cout << "game initialized:" << std::endl;
    std::cout << "  Active entities: " << entity_manager.active_count() << std::endl;
    std::cout << "  Target FPS: " << (opts.fps > 0 ? opts.fps : 60) << std::endl;
    std::cout << "  Window: 480x960" << std::endl;
    std::cout << "  Coordinate system: World space (0,0 = center), Y up" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  H                 - Toggle HUD visibility" << std::endl;
    std::cout << "  F1                - Toggle debug pause" << std::endl;
    std::cout << "  F2                - Single step (while paused)" << std::endl;
    std::cout << "  J                 - Dump this frame (before/after)" << std::endl;
    std::cout << "  T                 - Trace this frame (per-system)" << std::endl;
    std::cout << "  F10               - Toggle debug HUD" << std::endl;
    std::cout << "  ESC / Close       - Exit" << std::endl;

    // === Main Loop ===

    bool running = true;

    while (running) {
        // 1. Start frame timing
        timer.start_frame();

        // 2. Process input events and update Input components
        input_system.process_events(component_storage, running, blackboard);

        // 3. H key toggle for HUD visibility (edge detection via keyboard state)
        {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            static bool h_was_pressed = false;
            bool h_is_pressed = keys[SDL_SCANCODE_H];
            if (h_is_pressed && !h_was_pressed) {
                bool hud_visible = blackboard.get_or<bool>("hud_visible", true);
                blackboard.set("hud_visible", !hud_visible);
            }
            h_was_pressed = h_is_pressed;
        }

        // Debug key processing (edge detection via keyboard state)
        {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            bool f1_is_pressed = keys[SDL_SCANCODE_F1];
            bool f2_is_pressed = keys[SDL_SCANCODE_F2];
            bool f10_is_pressed = keys[SDL_SCANCODE_F10];

            // F1: toggle pause
            if (should_toggle(f1_was_pressed, f1_is_pressed)) {
                apply_pause_toggle(debug_paused, debug_hud_visible);
            }

            // F10: toggle debug HUD visibility
            if (should_toggle(f10_was_pressed, f10_is_pressed)) {
                debug_hud_visible = !debug_hud_visible;
            }

            // F2: single step (only when paused)
            if (should_step(debug_paused, f2_was_pressed, f2_is_pressed)) {
                step_requested = true;
            }

            // J: dump this frame, T: trace this frame (interactive, one-shot).
            // Arms the capture and, when paused, forces exactly one frame to advance.
            // Guarded by json_serializer so J/T are no-ops under --no-debug-keys.
            bool j_is_pressed = keys[SDL_SCANCODE_J];
            bool t_is_pressed = keys[SDL_SCANCODE_T];
            if (should_toggle(j_was_pressed, j_is_pressed) && json_serializer) {
                force_dump_this_step = true;
                if (debug_paused) step_requested = true;
            }
            if (should_toggle(t_was_pressed, t_is_pressed) && json_serializer) {
                force_trace_this_step = true;
                if (debug_paused) step_requested = true;
            }

            f1_was_pressed = f1_is_pressed;
            f2_was_pressed = f2_is_pressed;
            f10_was_pressed = f10_is_pressed;
            j_was_pressed = j_is_pressed;
            t_was_pressed = t_is_pressed;
        }

        // --- Key action injection from --keys CLI option ---
        // Clear arrow key flags injected on the previous frame (single-frame press)
        if (injected_arrow_last_frame) {
            auto entities = component_storage.entities_with_component<Input>();
            for (Entity e : entities) {
                auto input_opt = component_storage.get_component<Input>(e);
                if (input_opt.has_value()) {
                    Input& input = input_opt->get();
                    input.left = false;
                    input.right = false;
                    input.up = false;
                    input.down = false;
                    input.fire = false;
                }
            }
            injected_arrow_last_frame = false;
        }

        // Inject synthetic key presses for the current frame
        {
            uint64_t current_frame = timer.get_frame_count();
            bool arrow_injected = false;
            for (const auto& ka : opts.keys) {
                if (ka.frame != current_frame) continue;

                // Arrow keys: set Input component flags
                if (ka.key == "LEFT" || ka.key == "RIGHT" ||
                    ka.key == "UP" || ka.key == "DOWN") {
                    auto entities = component_storage.entities_with_component<Input>();
                    for (Entity e : entities) {
                        auto input_opt = component_storage.get_component<Input>(e);
                        if (input_opt.has_value()) {
                            Input& input = input_opt->get();
                            if (ka.key == "LEFT")  input.left = true;
                            if (ka.key == "RIGHT") input.right = true;
                            if (ka.key == "UP")    input.up = true;
                            if (ka.key == "DOWN")  input.down = true;
                        }
                    }
                    arrow_injected = true;
                }
                // Debug/control keys: simulate their effect directly
                else if (ka.key == "F1") {
                    apply_pause_toggle(debug_paused, debug_hud_visible);
                }
                else if (ka.key == "F2") {
                    if (debug_paused) {
                        step_requested = true;
                    }
                }
                else if (ka.key == "F10") {
                    debug_hud_visible = !debug_hud_visible;
                }
                else if (ka.key == "H") {
                    bool hud_visible = blackboard.get_or<bool>("hud_visible", true);
                    blackboard.set("hud_visible", !hud_visible);
                }
                else if (ka.key == "J") {
                    // Dump this frame (one-shot); advance one frame when paused.
                    if (json_serializer) {
                        force_dump_this_step = true;
                        if (debug_paused) step_requested = true;
                    }
                }
                else if (ka.key == "T") {
                    // Trace this frame (one-shot); advance one frame when paused.
                    if (json_serializer) {
                        force_trace_this_step = true;
                        if (debug_paused) step_requested = true;
                    }
                }
                else if (ka.key == "ESC") {
                    running = false;
                }
                else if (ka.key == "SPACE") {
                    auto entities = component_storage.entities_with_component<Input>();
                    for (Entity e : entities) {
                        auto input_opt = component_storage.get_component<Input>(e);
                        if (input_opt.has_value()) {
                            input_opt->get().fire = true;
                        }
                    }
                    arrow_injected = true;  // reuse the same clear mechanism
                }
            }
            injected_arrow_last_frame = arrow_injected;
        }

        // --- Mouse-hover injection from --hover CLI option ---
        // Override the polled cursor world position so flee behavior can be
        // triggered headlessly (no physical mouse). Mirrors --keys injection.
        {
            uint64_t current_frame = timer.get_frame_count();
            for (const auto& hv : opts.hovers) {
                if (hv.frame == current_frame) {
                    blackboard.set("mouse.x", static_cast<double>(hv.x));
                    blackboard.set("mouse.y", static_cast<double>(hv.y));
                }
            }
        }

        // --- Mouse-click injection from --clicks CLI option ---
        // Duck Pond does not currently consume mouse clicks, so this is a
        // functional no-op; it exists so the input-injection flag set matches
        // the other mouse-using classes (parse, round-trip, inject alike).
        {
            uint64_t current_frame = timer.get_frame_count();
            for (const auto& click : opts.clicks) {
                if (click.frame == current_frame) {
                    blackboard.set("mouse_click_x", static_cast<float>(click.x));
                    blackboard.set("mouse_click_y", static_cast<float>(click.y));
                    blackboard.set("mouse.clicked", true);
                }
            }
        }

        // Capture step_requested before simulation clears it (for timer logic)
        bool step_executed = step_requested;

        // Determine if this frame is a dump or trace frame
        uint64_t frame_for_dump_trace = timer.get_frame_count();
        // Frame-number-driven matching OR an interactive J/T capture armed this frame.
        bool is_dump_frame = (dump_trace_enabled &&
            std::find(opts.dump_frames.begin(), opts.dump_frames.end(), frame_for_dump_trace) != opts.dump_frames.end())
            || force_dump_this_step;
        bool is_trace_frame = (dump_trace_enabled &&
            std::find(opts.trace_frames.begin(), opts.trace_frames.end(), frame_for_dump_trace) != opts.trace_frames.end())
            || force_trace_this_step;

        // Simulation systems — skipped when paused, unless stepping
        if (!debug_paused || step_requested) {
            // Override delta_time for step frames (deterministic)
            if (step_requested) {
                blackboard.set("delta_time", step_delta_time(60.0));
            }

            // --- Frame trace logic ---
            if (is_trace_frame) {
                // Create trace subdirectory
                std::ostringstream trace_dir_ss;
                trace_dir_ss << log_dir << "/trace-"
                             << std::setw(6) << std::setfill('0') << frame_for_dump_trace;
                std::string trace_dir = trace_dir_ss.str();
                std::filesystem::create_directories(trace_dir);

                // Write 00-before.json (state before any systems)
                {
                    std::string json = json_serializer->serialize_state(
                        frame_for_dump_trace, "game", "070-duck-pond", "before", component_storage);
                    std::ofstream out(trace_dir + "/00-before.json");
                    out << json;
                }

                // System names and execution lambdas (matching simulation order)
                struct SystemStep {
                    std::string name;
                    std::function<void()> run;
                };

                std::vector<SystemStep> system_steps = {
                    {"RotationSystem", [&]() {
                        rotation_system.update(component_storage, blackboard);
                    }},
                    {"MovementSystem", [&]() {
                        movement_system.update(component_storage, blackboard);
                    }},
                    {"WrapSystem", [&]() {
                        wrap_system.update(component_storage, blackboard);
                    }},
                    {"CollisionSystem", [&]() {
                        collision_system.update(component_storage, blackboard);
                    }},
                    {"LifetimeSystem", [&]() {
                        lifetime_system.update(component_storage, blackboard);
                    }},
                    {"ScriptSystem", [&]() {
                        script_system.update(component_storage, entity_manager, blackboard);
                    }}
                };

                // Run each system individually with intermediate snapshots
                std::vector<std::string> system_names;
                for (size_t i = 0; i < system_steps.size(); ++i) {
                    system_steps[i].run();
                    system_names.push_back(system_steps[i].name);

                    // Write NN-SystemName.json
                    std::ostringstream step_filename;
                    step_filename << trace_dir << "/"
                                  << std::setw(2) << std::setfill('0') << (i + 1)
                                  << "-" << system_steps[i].name << ".json";
                    std::string json = json_serializer->serialize_state(
                        frame_for_dump_trace, "game", "070-duck-pond",
                        system_steps[i].name, component_storage);
                    std::ofstream out(step_filename.str());
                    out << json;
                }

                // Write summary.json
                size_t total_files = system_steps.size() + 2;  // before + per-system + summary
                std::string summary = json_serializer->serialize_trace_summary(
                    frame_for_dump_trace, "game", "070-duck-pond",
                    system_names, total_files,
                    current_timestamp());
                {
                    std::ofstream out(trace_dir + "/summary.json");
                    out << summary;
                }

                // Destroy entities marked with DestroyRequest
                destroy_marked_entities(entity_manager, component_storage);

                if (opts.verbose) {
                    std::cout << "  [Trace] Frame " << frame_for_dump_trace
                              << " → " << trace_dir << std::endl;
                }
            }
            // --- Frame dump logic ---
            else if (is_dump_frame) {
                // Serialize before-state
                std::string before_json = json_serializer->serialize_state(
                    frame_for_dump_trace, "game", "070-duck-pond", "before", component_storage);

                // Run all simulation systems normally
                rotation_system.update(component_storage, blackboard);
                movement_system.update(component_storage, blackboard);
                wrap_system.update(component_storage, blackboard);
                collision_system.update(component_storage, blackboard);
                lifetime_system.update(component_storage, blackboard);
                script_system.update(component_storage, entity_manager, blackboard);
                destroy_marked_entities(entity_manager, component_storage);

                // Serialize after-state
                std::string after_json = json_serializer->serialize_state(
                    frame_for_dump_trace, "game", "070-duck-pond", "after", component_storage);

                // Write both files to log directory (6-digit zero-padded)
                std::ostringstream before_path, after_path;
                before_path << log_dir << "/"
                            << std::setw(6) << std::setfill('0') << frame_for_dump_trace
                            << "-before.json";
                after_path << log_dir << "/"
                           << std::setw(6) << std::setfill('0') << frame_for_dump_trace
                           << "-after.json";
                {
                    std::ofstream out(before_path.str());
                    out << before_json;
                }
                {
                    std::ofstream out(after_path.str());
                    out << after_json;
                }

                if (opts.verbose) {
                    std::cout << "  [Dump] Frame " << frame_for_dump_trace
                              << " → " << before_path.str() << ", " << after_path.str() << std::endl;
                }
            }
            // --- Normal simulation (no dump/trace) ---
            else {
                
                // update game state
                game_state_system.update(component_storage, blackboard);

                // Spawn waves. Runs unconditionally (self-gates on game.state) so
                // it sees the retry transition, and before ScriptSystem so params
                // land before enemy.lua's on_init.
                enemy_spawn_system.update(component_storage, entity_manager, blackboard);

                if (GameStateSystem::is_playing(blackboard)) {
                // NEW (Class-110): player input -> velocity (arrow/WASD). Must
                // run before MovementSystem so the new velocity moves the ship
                // this same frame.
                player_control_system.update(component_storage);

                // NEW (Class-110): player fire -> spawn bullets (reads Input.fire,
                // enforces cooldown and the live-bullet cap). Ported from Class-050.
                bullet_spawn_system.update(component_storage, blackboard, entity_manager);

                // 1. Apply angular velocity to angle
                rotation_system.update(component_storage, blackboard);

                // 2. Update positions based on velocity and delta_time
                movement_system.update(component_storage, blackboard);

                // 3. Wrap entities at world boundaries
                wrap_system.update(component_storage, blackboard);

                // 4. Collision detection
                collision_system.update(component_storage, blackboard);

                // NEW (Class-110): turn this frame's collisions into consequences
                // — subtract Health, spend bullets, set game.state on player death.
                // Must run AFTER CollisionSystem (which writes CollidedWith) and
                // BEFORE destroy_marked_entities (which consumes DestroyRequest).
                collision_response_system.update(component_storage, blackboard);

                // NEW (Class-110): play the metal hit sound once per frame if any
                // player bullet struck an enemy/boss this frame. CollisionResponse
                // sets the flag; one clank per frame keeps a burst that lands
                // together from stacking into noise.
                if (blackboard.get_or<bool>("sfx.enemy_hit", false)) {
                    audio_system.play_sound("sfx_ghostHit.mp3", 0.05f);
                    blackboard.set<bool>("sfx.enemy_hit", false);
                }
                // Play any AudioSource components (play_on_add) queued this frame.
                audio_system.update(component_storage);

                // 5. Decrement lifetimes, mark expired entities
                lifetime_system.update(component_storage, blackboard);

                // NEW (Class-110): publish the player's CENTRE so enemy.lua's
                // aim_at_player can target it. Before ScriptSystem so enemies aim
                // at this frame's position; as double (Lua reads double first).
                for (Entity e : component_storage.entities_with_component<ShipTag>()) {
                    auto pp = component_storage.get_component<Position>(e);
                    auto ps = component_storage.get_component<Size>(e);
                    if (pp.has_value()) {
                        float hw = ps.has_value() ? ps->get().width  * 0.5f : 0.0f;
                        float hh = ps.has_value() ? ps->get().height * 0.5f : 0.0f;
                        blackboard.set<double>("player.x", pp->get().x + hw);
                        blackboard.set<double>("player.y", pp->get().y + hh);
                    }
                }

                // 6. Run Lua lifecycle hooks (on_init, on_update, on_collision, on_destroy)
                script_system.update(component_storage, entity_manager, blackboard);

                }  // end if (is_playing) — frozen at TITLE / WIN / GAME_OVER

                // 7. Destroy entities marked with DestroyRequest.
                // Runs unconditionally so a retry's cleanup still takes effect
                // on the frame the state changes.
                destroy_marked_entities(entity_manager, component_storage);
            }

            // Clear step flag after simulation completes
            if (step_requested) {
                apply_step_complete(step_requested, debug_paused);
            }

            
            // Clear interactive capture flags — one-shot per J/T press.
            force_dump_this_step = false;
            force_trace_this_step = false;

            for (Entity e : component_storage.entities_with_component<ShipTag>()) {
                auto hp_opt = component_storage.get_component<Health>(e);
                if (hp_opt.has_value()) {
                    const Health& hp = hp_opt->get();
                    blackboard.set<std::string>("hud.player_hp",
                        "HP: " + std::to_string(hp.current) + "/" + std::to_string(hp.max));
                }
            }
        }

        // Always-run systems (camera, rendering, HUD) — execute every frame
        // NOTE (Class-110): CameraControlSystem is intentionally NOT run. It pans
        // the camera on WASD, which collides with player movement (also WASD), and
        // this is a fixed single-screen shmup. The camera stays at its default
        // (lookat 0,0, zoom 1). The system + its engine tests are left untouched.

        // CameraSystem for debug panning
        camera_system.update(component_storage, blackboard);

        // Render frame
        // NEW (Class-110): advance sprite frame loops before drawing. Always-run
        // so the ship animates on the title/win screens too, not just in play.
        sprite_anim_system.update(component_storage, blackboard);

        // NEW (Class-110): tick hit flashes before drawing (always-run so a flash
        // still finishes and restores alpha if we leave PLAYING mid-flash).
        flash_system.update(component_storage, blackboard);

        render_system.clear_background();
        render_system.render(component_storage, blackboard);
        render_system.draw_world_border(blackboard);
        hud_system.render(component_storage, blackboard);
        debug_hud_system.render(debug_hud_visible, debug_paused, timer.get_frame_count(),
                               blackboard, entity_manager.active_count());

        // Screenshot capture BEFORE present — after present, the back buffer
        // contents are undefined (the swap chain has rotated to a new buffer)
        // and SDL_RenderReadPixels would return stale data from a previous frame.
        if (!opts.screenshot_frames.empty()) {
            uint64_t cur = timer.get_frame_count();
            if (std::find(opts.screenshot_frames.begin(), opts.screenshot_frames.end(), cur)
                    != opts.screenshot_frames.end()) {
                blackboard.set("screenshot_frame", cur);
            }
        }
        screenshot_system.update(blackboard);

        render_system.present();

        // Conditional timer end-frame
        if (!debug_paused || step_executed) {
            timer.end_frame();
        } else {
            timer.end_frame_no_advance();
        }
        timer.update_blackboard(blackboard);

        // Stop-frame check
        if (opts.stop_frame.has_value() &&
            timer.get_frame_count() >= opts.stop_frame.value()) {
            running = false;
        }

        // Verbose per-frame logging
        if (opts.verbose) {
            std::cout << "[Frame " << timer.get_frame_count() << "] "
                      << "dt=" << std::fixed << std::setprecision(6)
                      << blackboard.get_or<double>("delta_time", 0.0)
                      << " fps=" << std::fixed << std::setprecision(1)
                      << timer.get_fps()
                      << " paused=" << (debug_paused ? "true" : "false")
                      << std::endl;
        }
    }

    // Cleanup
    std::cout << std::endl;
    std::cout << "Shutting down..." << std::endl;
    std::cout << "  Total frames: " << timer.get_frame_count() << std::endl;
    std::cout << "  Final FPS: " << timer.get_fps() << std::endl;

    // Destroy SDL-backed resources BEFORE quitting the subsystems so every
    // deleter (SDL_DestroyTexture/TTF_CloseFont via ResourceManager, then
    // SDL_DestroyRenderer/SDL_DestroyWindow via the smart pointers) runs while
    // SDL/TTF are still initialized.
    // Order: resource_manager → renderer → window → TTF_Quit → SDL_Quit.
    resource_manager_ptr.reset();
    renderer.reset();
    window.reset();
    // NEW (Class-110): free audio handles (AudioSystem tracks, AudioManager audio)
    // BEFORE tearing down the mixer + MIX subsystem, or those frees hit a dead mixer.
    audio_system_ptr.reset();
    audio_manager_ptr.reset();
    if (mixer) {
        MIX_DestroyMixer(mixer);
        MIX_Quit();
    }
    TTF_Quit();
    SDL_Quit();

    return 0;
}
