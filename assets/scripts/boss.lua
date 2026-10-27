-- boss.lua
-- Multi-phase boss. One entity, one Script: it reads its own Health via
-- engine.get_health(id) and switches bullet pattern by HP fraction
-- Phases live as inline Lua tables (see PHASES) rather than Blackboard params,
--

local timers    = {}   -- per-entity fire timer
local spin_rot  = {}   -- per-entity ring-rotation accumulator (reset per phase)
local states    = {}   -- per-entity lifecycle state
local phase_ix  = {}   -- per-entity active phase index (to detect phase change)
local home_x    = {}   -- per-entity weave anchor (re-set on each phase change)
local strafe_t  = {}   -- per-entity weave-time accumulator

-- Boss movement (entry glide). Constants, not params — it's one entity.
local ENTRY_SPEED = 60      -- px/sec descent while entering
local HOLD_Y      = 320     -- y it stops at and starts fighting

-- The boss uses the first phase whose `min_frac` is <= current HP fraction.
-- Each phase is one burst shape (same knobs as enemy.lua: arc_span / spin / aim).
-- strafe_amp/strafe_speed are optional per phase (omit = stationary). Anchor
-- re-centres on each phase change, so the boss weaves around wherever it is.
local PHASES = {
    -- full ring, slow, not aimed, gentle weave
    { min_frac = 0.66, fire_interval = 0.3, burst_count = 20, arc_span = 6.2831853, spin = 0.20, aim = 0, speed = 300, strafe_amp = 90,  strafe_speed = 0.8 },
    -- spiral (small burst + spin), wider weave
    { min_frac = 0.33, fire_interval = 0.05, burst_count = 5,  arc_span = 6.2831853, spin = 1.0, aim = 0, speed = 450, strafe_amp = 0, strafe_speed = 0.0 },
    -- desperate: aimed fan, fast weave
    { min_frac = 0.00, fire_interval = 0.80, burst_count = 12,  arc_span = 1.00,      spin = 0.00, aim = 1, speed = 400, strafe_amp = 160, strafe_speed = 1.8 },
}
local BULLET_SIZE = 32
local BULLET_LIFE = 4.0

-- ===== shared helpers (burst_angles still mirrors enemy.lua) =====

local function centre_of(id)
    local x, y = engine.get_position(id)
    if x == nil then return nil, nil end
    local w, h = engine.get_size(id)
    return x + (w or 0) / 2, y + (h or 0) / 2
end

local function spawn_bullet(cx, cy, dx, dy, p)
    engine.fire_bullet{
        x = cx, y = cy,
        vx = dx * p.speed, vy = dy * p.speed,
        size = p.size,
        layer = 8, mask = 1,   -- enemy bullet -> player
        lifetime = p.life,
        sprite = "boss_bullet.png",
        r = 255, g = 90, b = 160, a = 255,
    }
end

-- aim is 0 (fire down) or 1 (toward the player); passed per-phase.
local function base_angle_for(id, cx, cy, aim)
    local DOWN = -math.pi / 2
    if aim ~= 1 then return DOWN end
    local px = engine.get_blackboard("player.x")
    local py = engine.get_blackboard("player.y")
    if px == nil or py == nil then return DOWN end
    return math.atan(py - cy, px - cx)
end

local function burst_angles(base, count, arc)
    local out = {}
    if count <= 1 then out[1] = base; return out end
    if arc >= 2 * math.pi - 1e-6 then
        for i = 0, count - 1 do out[#out + 1] = base + (i / count) * 2 * math.pi end
    else
        for i = 0, count - 1 do out[#out + 1] = base - arc / 2 + (i / (count - 1)) * arc end
    end
    return out
end

-- Index of the active phase from current HP fraction (falls back safely).
local function current_phase_index(id)
    local cur, max = engine.get_health(id)
    if cur == nil or max == nil or max <= 0 then return 1 end
    local frac = cur / max
    for i, ph in ipairs(PHASES) do
        if frac >= ph.min_frac then return i end
    end
    return #PHASES
end

function on_init(id)
    timers[id]   = 0.0
    spin_rot[id] = 0.0
    strafe_t[id] = 0.0
    home_x[id]   = nil
    states[id]   = "entering"
    phase_ix[id] = nil   -- first active frame sets it (and resets spin_rot)
    engine.add_component(id, "IgnoreWorldBounds")   -- enters from above the arena
    engine.set_velocity(id, 0, -ENTRY_SPEED)
end

function on_update(id, dt)
    local cx, cy = centre_of(id)
    if cx == nil then return end

    -- Entering: descend to the hold line, then start fighting.
    if states[id] == "entering" then
        local _, y = engine.get_position(id)
        if y <= HOLD_Y then
            engine.set_velocity(id, 0, 0)
            states[id] = "active"
        end
        return
    end

    -- Active: pick phase by HP, fire its pattern on its own interval.
    local idx = current_phase_index(id)
    if phase_ix[id] ~= idx then
        -- New phase: reset rotation (so aim/spin isn't thrown off by earlier
        -- phases) and re-anchor the weave to the current x (so it starts from
        -- centre with no positional jump).
        spin_rot[id] = 0.0
        strafe_t[id] = 0.0
        home_x[id]   = (engine.get_position(id))
        phase_ix[id] = idx
    end
    local ph = PHASES[idx]

    -- Weave horizontally around the anchor (strafe_amp nil/0 = stationary).
    local amp = ph.strafe_amp or 0
    if amp ~= 0 then
        strafe_t[id] = strafe_t[id] + dt
        local nx = home_x[id] + amp * math.sin(strafe_t[id] * (ph.strafe_speed or 1.5))
        local _, y = engine.get_position(id)
        engine.set_position(id, nx, y)
        cx, cy = centre_of(id)   -- fire from the moved position
    end

    if ph.fire_interval <= 0 then return end

    local p = { size = BULLET_SIZE, speed = ph.speed, life = BULLET_LIFE }
    timers[id] = timers[id] + dt
    while timers[id] >= ph.fire_interval do
        timers[id] = timers[id] - ph.fire_interval
        local base = base_angle_for(id, cx, cy, ph.aim) + spin_rot[id]
        for _, angle in ipairs(burst_angles(base, math.floor(ph.burst_count), ph.arc_span)) do
            spawn_bullet(cx, cy, math.cos(angle), math.sin(angle), p)
        end
        spin_rot[id] = spin_rot[id] + ph.spin
    end
end

function on_destroy(id)
    timers[id]   = nil
    spin_rot[id] = nil
    states[id]   = nil
    phase_ix[id] = nil
    home_x[id]   = nil
    strafe_t[id] = nil
end
