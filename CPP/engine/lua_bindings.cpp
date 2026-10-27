#include "lua_bindings.hpp"
#include "engine/ecs/component_storage.hpp"
#include "engine/ecs/entity_manager.hpp"
#include "engine/ecs/blackboard.hpp"
#include "engine/ecs/bullet_factory.hpp"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <cmath>
#include <algorithm>
#include <string>

// Registry key constants
static const char* KEY_COMPONENT_STORAGE = "engine.component_storage";
static const char* KEY_ENTITY_MANAGER    = "engine.entity_manager";
static const char* KEY_BLACKBOARD        = "engine.blackboard";

// ============================================================================
// Helper functions to retrieve engine pointers from Lua registry
// ============================================================================

static ComponentStorage* get_storage(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, KEY_COMPONENT_STORAGE);
    auto* ptr = static_cast<ComponentStorage*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return ptr;
}

static EntityManager* get_entity_manager(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, KEY_ENTITY_MANAGER);
    auto* ptr = static_cast<EntityManager*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return ptr;
}

static Blackboard* get_blackboard(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, KEY_BLACKBOARD);
    auto* ptr = static_cast<Blackboard*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return ptr;
}

// ============================================================================
// Task 2: Component read/write bindings
// ============================================================================

// --- Position ---

static int l_get_position(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto pos = storage->get_component<Position>(id);
    if (!pos.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, pos->get().x);
    lua_pushnumber(L, pos->get().y);
    return 2;
}

static int l_set_position(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    auto* storage = get_storage(L);
    storage->add_component<Position>(id, Position{x, y});
    return 0;
}

// --- Velocity ---

static int l_get_velocity(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto vel = storage->get_component<Velocity>(id);
    if (!vel.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, vel->get().dx);
    lua_pushnumber(L, vel->get().dy);
    return 2;
}

static int l_set_velocity(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float dx = static_cast<float>(luaL_checknumber(L, 2));
    float dy = static_cast<float>(luaL_checknumber(L, 3));
    auto* storage = get_storage(L);
    storage->add_component<Velocity>(id, Velocity{dx, dy});
    return 0;
}

// --- Rotation ---

static int l_get_rotation(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto rot = storage->get_component<Rotation>(id);
    if (!rot.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, rot->get().angle);
    return 1;
}

static int l_set_rotation(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float angle = static_cast<float>(luaL_checknumber(L, 2));
    auto* storage = get_storage(L);
    storage->add_component<Rotation>(id, Rotation{angle, 0.0f});
    return 0;
}

// --- Size ---

static int l_get_size(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto sz = storage->get_component<Size>(id);
    if (!sz.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, sz->get().width);
    lua_pushnumber(L, sz->get().height);
    return 2;
}

// engine.get_health(id) -> current, max   (nil if the entity has no Health)
// Reading one's own HP is a legitimate script read (like get_position); only
// WRITING game rules (applying damage) is kept in C++. boss.lua uses this to
// pick its phase.
static int l_get_health(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto hp = storage->get_component<Health>(id);
    if (!hp.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, hp->get().current);
    lua_pushinteger(L, hp->get().max);
    return 2;
}

static int l_set_size(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    float w = static_cast<float>(luaL_checknumber(L, 2));
    float h = static_cast<float>(luaL_checknumber(L, 3));
    auto* storage = get_storage(L);
    storage->add_component<Size>(id, Size{w, h});
    return 0;
}

// --- Color ---

static int l_get_color(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto col = storage->get_component<Color>(id);
    if (!col.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, col->get().r);
    lua_pushinteger(L, col->get().g);
    lua_pushinteger(L, col->get().b);
    lua_pushinteger(L, col->get().a);
    return 4;
}

static int l_set_color(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    int r = static_cast<int>(luaL_checkinteger(L, 2));
    int g = static_cast<int>(luaL_checkinteger(L, 3));
    int b = static_cast<int>(luaL_checkinteger(L, 4));
    int a = static_cast<int>(luaL_checkinteger(L, 5));
    auto* storage = get_storage(L);
    storage->add_component<Color>(id, Color{
        static_cast<uint8_t>(std::clamp(r, 0, 255)),
        static_cast<uint8_t>(std::clamp(g, 0, 255)),
        static_cast<uint8_t>(std::clamp(b, 0, 255)),
        static_cast<uint8_t>(std::clamp(a, 0, 255))
    });
    return 0;
}

// --- Input (read-only) ---

static int l_get_input(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    auto inp = storage->get_component<Input>(id);
    if (!inp.has_value()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, inp->get().up);
    lua_pushboolean(L, inp->get().down);
    lua_pushboolean(L, inp->get().left);
    lua_pushboolean(L, inp->get().right);
    lua_pushboolean(L, inp->get().fire);
    return 5;
}

// ============================================================================
// Task 3: Blackboard bindings
// ============================================================================

static int l_get_blackboard(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    auto* bb = get_blackboard(L);

    // Try double
    try {
        double val = bb->get<double>(key);
        lua_pushnumber(L, val);
        return 1;
    } catch (...) {}

    // Try float (gamedata_loader stores world bounds as float)
    try {
        float val = bb->get<float>(key);
        lua_pushnumber(L, static_cast<double>(val));
        return 1;
    } catch (...) {}

    // Try int
    try {
        int val = bb->get<int>(key);
        lua_pushinteger(L, val);
        return 1;
    } catch (...) {}

    // Try std::string
    try {
        std::string val = bb->get<std::string>(key);
        lua_pushstring(L, val.c_str());
        return 1;
    } catch (...) {}

    // Try bool
    try {
        bool val = bb->get<bool>(key);
        lua_pushboolean(L, val);
        return 1;
    } catch (...) {}

    // Key not found or unsupported type
    lua_pushnil(L);
    return 1;
}

static int l_set_blackboard(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    auto* bb = get_blackboard(L);

    if (lua_isboolean(L, 2)) {
        bool val = lua_toboolean(L, 2);
        bb->set(key, val);
    } else if (lua_isnumber(L, 2)) {
        double val = lua_tonumber(L, 2);
        bb->set(key, val);
    } else if (lua_isstring(L, 2)) {
        std::string val = lua_tostring(L, 2);
        bb->set(key, val);
    }

    return 0;
}

// ============================================================================
// Task 4: Entity lifecycle bindings
// ============================================================================

static int l_create_entity(lua_State* L) {
    auto* em = get_entity_manager(L);
    Entity id = em->create_entity();
    lua_pushinteger(L, static_cast<lua_Integer>(id));
    return 1;
}

static int l_destroy_entity(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    auto* storage = get_storage(L);
    storage->add_component<DestroyRequest>(id, DestroyRequest{});
    return 0;
}

static int l_add_component(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    const char* type_name = luaL_checkstring(L, 2);
    auto* storage = get_storage(L);

    std::string type(type_name);

    if (type == "Position") {
        lua_getfield(L, 3, "x");
        float x = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "y");
        float y = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        storage->add_component<Position>(id, Position{x, y});
    } else if (type == "Size") {
        lua_getfield(L, 3, "width");
        float w = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "height");
        float h = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        storage->add_component<Size>(id, Size{w, h});
    } else if (type == "Color") {
        lua_getfield(L, 3, "r");
        int r = static_cast<int>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "g");
        int g = static_cast<int>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "b");
        int b = static_cast<int>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "a");
        int a = static_cast<int>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        storage->add_component<Color>(id, Color{
            static_cast<uint8_t>(std::clamp(r, 0, 255)),
            static_cast<uint8_t>(std::clamp(g, 0, 255)),
            static_cast<uint8_t>(std::clamp(b, 0, 255)),
            static_cast<uint8_t>(std::clamp(a, 0, 255))
        });
    } else if (type == "Velocity") {
        lua_getfield(L, 3, "dx");
        float dx = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "dy");
        float dy = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        storage->add_component<Velocity>(id, Velocity{dx, dy});
    } else if (type == "Rotation") {
        lua_getfield(L, 3, "angle");
        float angle = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        storage->add_component<Rotation>(id, Rotation{angle, 0.0f});
    } else if (type == "Collider") {
        lua_getfield(L, 3, "width");
        float w = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "height");
        float h = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "layer");
        uint8_t layer = static_cast<uint8_t>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, 3, "mask");
        uint8_t mask = static_cast<uint8_t>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        storage->add_component<Collider>(id, Collider{w, h, layer, mask});
    } else if (type == "Lifetime") {
        lua_getfield(L, 3, "remaining");
        float remaining = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 1);
        storage->add_component<Lifetime>(id, Lifetime{remaining});
    } else if (type == "WrapAround") {
        storage->add_component<WrapAround>(id, WrapAround{});
    } 
    else if (type == "BulletTag") {
        storage->add_component<BulletTag>(id, BulletTag{});

    }
    else if (type == "IgnoreWorldBounds") {
        storage->add_component<IgnoreWorldBounds>(id, IgnoreWorldBounds{});
    }
    else {
        return luaL_error(L, "Unknown component type: %s", type_name);
    }

    return 0;
}

static int l_has_component(lua_State* L) {
    Entity id = static_cast<Entity>(luaL_checkinteger(L, 1));
    const char* type_name = luaL_checkstring(L, 2);
    auto* storage = get_storage(L);

    std::string type(type_name);
    bool result = false;

    if (type == "Position") {
        result = storage->has_component<Position>(id);
    } else if (type == "Size") {
        result = storage->has_component<Size>(id);
    } else if (type == "Color") {
        result = storage->has_component<Color>(id);
    } else if (type == "Velocity") {
        result = storage->has_component<Velocity>(id);
    } else if (type == "Rotation") {
        result = storage->has_component<Rotation>(id);
    } else if (type == "Collider") {
        result = storage->has_component<Collider>(id);
    } else if (type == "Lifetime") {
        result = storage->has_component<Lifetime>(id);
    } else if (type == "WrapAround") {
        result = storage->has_component<WrapAround>(id);
    } else if (type == "Input") {
        result = storage->has_component<Input>(id);
    } else if (type == "Script") {
        result = storage->has_component<Script>(id);
    } else if (type == "DestroyRequest") {
        result = storage->has_component<DestroyRequest>(id);
    } else if (type == "CollidedWith") {
        result = storage->has_component<CollidedWith>(id);
    } else {
        return luaL_error(L, "Unknown component type: %s", type_name);
    }

    lua_pushboolean(L, result);
    return 1;
}

// ============================================================================
// Task 5: Entity query binding
// ============================================================================

static int l_entities_with(lua_State* L) {
    const char* type_name = luaL_checkstring(L, 1);
    auto* storage = get_storage(L);

    std::string type(type_name);
    std::vector<Entity> entities;

    if (type == "Position") {
        entities = storage->entities_with_component<Position>();
    } else if (type == "Size") {
        entities = storage->entities_with_component<Size>();
    } else if (type == "Color") {
        entities = storage->entities_with_component<Color>();
    } else if (type == "Velocity") {
        entities = storage->entities_with_component<Velocity>();
    } else if (type == "Rotation") {
        entities = storage->entities_with_component<Rotation>();
    } else if (type == "Collider") {
        entities = storage->entities_with_component<Collider>();
    } else if (type == "Lifetime") {
        entities = storage->entities_with_component<Lifetime>();
    } else if (type == "WrapAround") {
        entities = storage->entities_with_component<WrapAround>();
    } else if (type == "Input") {
        entities = storage->entities_with_component<Input>();
    } else if (type == "Script") {
        entities = storage->entities_with_component<Script>();
    } else if (type == "DestroyRequest") {
        entities = storage->entities_with_component<DestroyRequest>();
    } else if (type == "CollidedWith") {
        entities = storage->entities_with_component<CollidedWith>();
    } else {
        return luaL_error(L, "Unknown component type: %s", type_name);
    }

    lua_newtable(L);
    for (size_t i = 0; i < entities.size(); ++i) {
        lua_pushinteger(L, static_cast<lua_Integer>(entities[i]));
        lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
    }
    return 1;
}

// ============================================================================
// Task 6: Math helper bindings
// ============================================================================

static int l_direction_from_angle(lua_State* L) {
    double angle = luaL_checknumber(L, 1);
    lua_pushnumber(L, std::cos(angle));
    lua_pushnumber(L, std::sin(angle));
    return 2;
}

static int l_distance(lua_State* L) {
    double x1 = luaL_checknumber(L, 1);
    double y1 = luaL_checknumber(L, 2);
    double x2 = luaL_checknumber(L, 3);
    double y2 = luaL_checknumber(L, 4);
    double dx = x2 - x1;
    double dy = y2 - y1;
    lua_pushnumber(L, std::sqrt(dx * dx + dy * dy));
    return 1;
}

static int l_get_entity_count(lua_State* L) {
    // No arguments needed
    // Retrieve the EntityManager from the registry
    // Call active_count() to get the number of active entities
    // Push the count as an integer
    // Return 1 (one value pushed)
    auto* em = get_entity_manager(L);
    int count = em->active_count();

    lua_pushinteger(L, count);
    return 1;
}

// ============================================================================
// Bullet spawning: one recipe shared with the player's BulletSpawnSystem
// ============================================================================

// engine.fire_bullet{ x, y, vx, vy, size, layer, mask, lifetime,
//                     r, g, b, a, sprite, angle } -> entity id
//
// Builds a bullet through the shared C++ BulletSpec factory so enemy/boss
// scripts and the player construct the identical entity. Every field is
// optional; `sprite` (Images) and `angle` (Rotation) are only added if present.
static int l_fire_bullet(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    auto* storage = get_storage(L);
    auto* em = get_entity_manager(L);

    // Read an optional numeric field from the spec table, else use `def`.
    auto num = [L](const char* k, double def) -> double {
        lua_getfield(L, 1, k);
        double v = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : def;
        lua_pop(L, 1);
        return v;
    };
    auto u8 = [&num](const char* k, double def) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(static_cast<int>(num(k, def)), 0, 255));
    };

    BulletSpec spec;
    spec.x        = static_cast<float>(num("x", 0.0));
    spec.y        = static_cast<float>(num("y", 0.0));
    spec.vx       = static_cast<float>(num("vx", 0.0));
    spec.vy       = static_cast<float>(num("vy", 0.0));
    spec.size     = static_cast<float>(num("size", 8.0));
    spec.layer    = static_cast<uint8_t>(num("layer", 0.0));
    spec.mask     = static_cast<uint8_t>(num("mask", 0.0));
    spec.lifetime = static_cast<float>(num("lifetime", 4.0));
    spec.color    = Color{u8("r", 255.0), u8("g", 255.0), u8("b", 255.0), u8("a", 255.0)};

    lua_getfield(L, 1, "sprite");
    if (lua_isstring(L, -1)) spec.sprite = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "angle");
    if (lua_isnumber(L, -1)) {
        spec.has_rotation = true;
        spec.angle = static_cast<float>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    Entity b = spawn_bullet(*storage, *em, spec);
    lua_pushinteger(L, static_cast<lua_Integer>(b));
    return 1;
}

// ============================================================================
// Registration and pointer storage
// ============================================================================

void register_bindings(lua_State* L) {
    lua_newtable(L);

    // Component bindings
    lua_pushcfunction(L, l_get_position);
    lua_setfield(L, -2, "get_position");
    lua_pushcfunction(L, l_set_position);
    lua_setfield(L, -2, "set_position");

    lua_pushcfunction(L, l_get_velocity);
    lua_setfield(L, -2, "get_velocity");
    lua_pushcfunction(L, l_set_velocity);
    lua_setfield(L, -2, "set_velocity");

    lua_pushcfunction(L, l_get_rotation);
    lua_setfield(L, -2, "get_rotation");
    lua_pushcfunction(L, l_set_rotation);
    lua_setfield(L, -2, "set_rotation");

    lua_pushcfunction(L, l_get_health);
    lua_setfield(L, -2, "get_health");

    lua_pushcfunction(L, l_get_size);
    lua_setfield(L, -2, "get_size");
    lua_pushcfunction(L, l_set_size);
    lua_setfield(L, -2, "set_size");

    lua_pushcfunction(L, l_get_color);
    lua_setfield(L, -2, "get_color");
    lua_pushcfunction(L, l_set_color);
    lua_setfield(L, -2, "set_color");

    lua_pushcfunction(L, l_get_input);
    lua_setfield(L, -2, "get_input");

    // Blackboard bindings
    lua_pushcfunction(L, l_get_blackboard);
    lua_setfield(L, -2, "get_blackboard");
    lua_pushcfunction(L, l_set_blackboard);
    lua_setfield(L, -2, "set_blackboard");

    // Entity lifecycle bindings
    lua_pushcfunction(L, l_create_entity);
    lua_setfield(L, -2, "create_entity");
    lua_pushcfunction(L, l_destroy_entity);
    lua_setfield(L, -2, "destroy_entity");
    lua_pushcfunction(L, l_add_component);
    lua_setfield(L, -2, "add_component");
    lua_pushcfunction(L, l_has_component);
    lua_setfield(L, -2, "has_component");
    lua_pushcfunction(L, l_fire_bullet);
    lua_setfield(L, -2, "fire_bullet");

    // Entity query binding
    lua_pushcfunction(L, l_entities_with);
    lua_setfield(L, -2, "entities_with");

    // Math helper bindings
    lua_pushcfunction(L, l_direction_from_angle);
    lua_setfield(L, -2, "direction_from_angle");
    lua_pushcfunction(L, l_distance);
    lua_setfield(L, -2, "distance");


    // get entity count binding
    lua_pushcfunction(L, l_get_entity_count);
    lua_setfield(L, -2, "get_entity_count");

    lua_setglobal(L, "engine");

}

void store_engine_pointers(lua_State* L,
                           ComponentStorage* storage,
                           EntityManager* entity_manager,
                           Blackboard* blackboard) {
    lua_pushlightuserdata(L, storage);
    lua_setfield(L, LUA_REGISTRYINDEX, KEY_COMPONENT_STORAGE);

    lua_pushlightuserdata(L, entity_manager);
    lua_setfield(L, LUA_REGISTRYINDEX, KEY_ENTITY_MANAGER);

    lua_pushlightuserdata(L, blackboard);
    lua_setfield(L, LUA_REGISTRYINDEX, KEY_BLACKBOARD);
}
