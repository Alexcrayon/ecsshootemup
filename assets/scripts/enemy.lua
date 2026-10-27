-- enemy.lua
-- One parameterized behavior script for every enemy type 
-- Movement and firing live in the same file because an entity can only
-- have one Script component, and the sandbox removes `require`, so scripts
-- cannot be composed. Instead of separate files per pattern, the differences
-- are parameters:
--
--   arc_span        2*pi = full ring, smaller = a fan/spread
--   spin_per_burst 0 = fixed ring, non-zero = spiral
--   aim_at_player   1 = aim the burst at the player, 0 = fire downward
--
-- Parameters are seeded into per-entity Blackboard keys (entity.<id>.<name>)
--
-- Lifecycle: "entering" (descend from off-screen) -> "active" (hold and fire).

local timers  = {}  -- per-entity fire timer
local phases  = {}  -- per-entity ring rotation (accumulates spin_per_volley)
local states  = {}  -- per-entity lifecycle state
local home_x  = {}  -- per-entity x it went active at (centre of the weave)
local strafe_t = {} -- per-entity weave-time accumulator

local DEFAULTS = {
    -- movement
    entry_speed     = 90,        -- pixels/second descent while entering
    hold_y          = 300,       -- y at which the enemy stops and goes active
    strafe_amp      = 0,         -- horizontal weave amplitude in px (0 = still)
    strafe_speed    = 1.5,       -- weave angular speed in radians/second
    -- firing
    fire_interval   = 1.0,       -- seconds between bursts
    burst_count     = 12,        -- bullets per burst
    arc_span        = 6.2831853, -- 2*pi: full ring
    spin_per_burst = 0,       -- radians added to the base angle each burst
    aim_at_player   = 1,         -- 0/1 rather than a boolean, so the value type
                                 -- stays consistent when it comes from JSON
    bullet_speed    = 180,
    bullet_size     = 32,
    bullet_life     = 4.0,
}

local function key(id, name)
    return "entity." .. id .. "." .. name
end

-- Seed a per-entity key only if nothing has set it yet, so a spawner wins.
local function seed(id, name, value)
    local k = key(id, name)
    if engine.get_blackboard(k) == nil then
        engine.set_blackboard(k, value)
    end
end

-- No `or default` fallback on purpose: every key is seeded in on_init, so a nil
-- here is a real bug and should fail loudly instead of silently substituting.
local function cfg(id, name)
    return engine.get_blackboard(key(id, name))
end

-- Centre of the entity in world space (Position is its bottom-left corner).
local function centre_of(id)
    local x, y = engine.get_position(id)
    if x == nil then return nil, nil end
    local w, h = engine.get_size(id)
    return x + (w or 0) / 2, y + (h or 0) / 2
end

-- spawn bullets
local function spawn_bullet(cx, cy, dx, dy, p)
    engine.fire_bullet{
        x = cx, y = cy,
        vx = dx * p.speed, vy = dy * p.speed,
        size = p.size,
        layer = 8, mask = 1,
        lifetime = p.life,
        sprite = "enemy_bullet.png",
        r = 255, g = 140, b = 60, a = 255,
    }
end

-- The angle the burst is built around.
local function base_angle(id, cx, cy)
    local DOWN = -math.pi / 2   -- toward the player's side of the screen
    if cfg(id, "aim_at_player") ~= 1 then
        return DOWN
    end
    -- Requires C++ to publish player.x / player.y. Falls back to DOWN until it
    -- does, so turning this on early degrades gracefully instead of erroring.
    local px = engine.get_blackboard("player.x")
    local py = engine.get_blackboard("player.y")
    if px == nil or py == nil then
        return DOWN
    end
    return math.atan(py - cy, px - cx)
end

-- Angles for one burst: evenly spaced around a full ring, or spread across
-- `arc` centred on `base`.
local function burst_angles(base, count, arc)
    local out = {}
    if count <= 1 then
        out[1] = base
        return out
    end
    if arc >= 2 * math.pi - 1e-6 then
        -- Full ring: divide by count so the first and last do not overlap.
        for i = 0, count - 1 do
            out[#out + 1] = base + (i / count) * 2 * math.pi
        end
    else
        -- Fan: divide by count-1 so the outermost bullets land on the edges.
        for i = 0, count - 1 do
            out[#out + 1] = base - arc / 2 + (i / (count - 1)) * arc
        end
    end
    return out
end

function on_init(id)
    timers[id] = 0.0
    phases[id] = 0.0
    states[id] = "entering"
    strafe_t[id] = 0.0
    home_x[id] = nil   -- set when it reaches the hold line

    for name, value in pairs(DEFAULTS) do
        seed(id, name, value)
    end

    -- The enemy starts above the arena, so opt out of MovementSystem's
    -- world-bound clamping or it would be snapped inside on its first move.
    engine.add_component(id, "IgnoreWorldBounds")

    -- Descend. set_velocity creates the Velocity component if absent, which is
    -- also what makes MovementSystem start processing this entity.
    engine.set_velocity(id, 0, -cfg(id, "entry_speed"))
end

function on_update(id, dt)
    local cx, cy = centre_of(id)
    if cx == nil then return end

    -- --- Entering: descend until the hold line, then stop and go active ---
    if states[id] == "entering" then
        local x, y = engine.get_position(id)
        if y <= cfg(id, "hold_y") then
            engine.set_velocity(id, 0, 0)   -- weave (if any) drives motion now
            states[id] = "active"
            home_x[id] = x                  -- weave oscillates around this lane
        end
        return                              -- hold fire until in position
    end

    -- --- Active: weave horizontally, then fire ---
    local amp = cfg(id, "strafe_amp")
    if amp ~= 0 then
        if home_x[id] == nil then home_x[id] = (engine.get_position(id)) end
        strafe_t[id] = strafe_t[id] + dt
        local nx = home_x[id] + amp * math.sin(strafe_t[id] * cfg(id, "strafe_speed"))
        local _, y = engine.get_position(id)
        engine.set_position(id, nx, y)
        cx, cy = centre_of(id)   -- fire from the moved position
    end

    -- Fire on a fixed interval.
    local interval = cfg(id, "fire_interval")
    -- Guard: a zero/negative interval would spin the loop below forever.
    if interval <= 0 then return end

    local p = {
        size  = cfg(id, "bullet_size"),
        speed = cfg(id, "bullet_speed"),
        life  = cfg(id, "bullet_life"),
    }
    local count = math.floor(cfg(id, "burst_count"))
    local arc   = cfg(id, "arc_span")
    local spin  = cfg(id, "spin_per_burst")

    -- Accumulate real time and emit one burst per elapsed interval.
    -- Subtracting the interval keeps the remainder, so burst spacing stays
    -- frame-rate independent.
    timers[id] = timers[id] + dt
    while timers[id] >= interval do
        timers[id] = timers[id] - interval

        local base = base_angle(id, cx, cy) + phases[id]
        for _, angle in ipairs(burst_angles(base, count, arc)) do
            spawn_bullet(cx, cy, math.cos(angle), math.sin(angle), p)
        end

        phases[id] = phases[id] + spin
    end
end

function on_destroy(id)
    timers[id] = nil
    phases[id] = nil
    states[id] = nil
    home_x[id] = nil
    strafe_t[id] = nil
end
