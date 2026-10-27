-- wander.lua
-- Random wandering behavior for fish and dragonfly entities.
-- Sets velocity each frame; MovementSystem handles position + boundary clamping.
-- Speed is inferred from entity size: smaller entities move faster.

function on_init(id)
    math.randomseed(id)

    local w, h = engine.get_size(id)

    -- Smaller entities move faster: base 40, bonus up to 80 for tiny entities
    local default_speed = 40 + (1 - math.min(w, 50) / 50) * 80

    -- Only set speed if not already pre-configured in the blackboard
    local existing_speed = engine.get_blackboard("entity." .. id .. ".wander_speed")
    if existing_speed == nil then
        engine.set_blackboard("entity." .. id .. ".wander_speed", default_speed)
    end

    -- Random initial direction (unit vector from random angle)
    local angle = math.random() * 2 * math.pi
    engine.set_blackboard("entity." .. id .. ".wander_dx", math.cos(angle))
    engine.set_blackboard("entity." .. id .. ".wander_dy", math.sin(angle))
    engine.set_blackboard("entity." .. id .. ".wander_timer", 1.0 + math.random() * 2.0)

    -- Scatter state initialization (for duck-collision flee behavior)
    engine.set_blackboard("entity." .. id .. ".fleeing", false)
    engine.set_blackboard("entity." .. id .. ".scatter_timer", 0)
end

function on_update(id, dt)
    local fleeing = engine.get_blackboard("entity." .. id .. ".fleeing")

    if fleeing then
        -- Scatter behavior: move away from duck collision point
        local scatter_timer = engine.get_blackboard("entity." .. id .. ".scatter_timer")
        scatter_timer = scatter_timer - dt

        -- Cancel scatter if timer expired OR entity hit a world edge
        local at_edge = false
        local x, y = engine.get_position(id)
        local wx = engine.get_blackboard("world.x")
        local wy = engine.get_blackboard("world.y")
        local ww = engine.get_blackboard("world.width")
        local wh = engine.get_blackboard("world.height")
        if wx ~= nil then
            local ew, eh = engine.get_size(id)
            at_edge = (x <= wx) or (x >= wx + ww - ew) or
                      (y <= wy) or (y >= wy + wh - eh)
        end

        if scatter_timer <= 0 or at_edge then
            -- Resume normal wander
            engine.set_blackboard("entity." .. id .. ".fleeing", false)
            engine.set_blackboard("entity." .. id .. ".scatter_timer", 0)
            -- Fall through to normal wander below
        else
            engine.set_blackboard("entity." .. id .. ".scatter_timer", scatter_timer)

            local sdx = engine.get_blackboard("entity." .. id .. ".scatter_dx")
            local sdy = engine.get_blackboard("entity." .. id .. ".scatter_dy")
            local scatter_speed = 120  -- faster than normal wander

            engine.set_velocity(id, sdx * scatter_speed, sdy * scatter_speed)
            engine.set_rotation(id, math.atan(sdy, sdx))
            return  -- skip normal wander logic
        end
    end

    -- Normal wander logic
    local speed = engine.get_blackboard("entity." .. id .. ".wander_speed")
    local dx = engine.get_blackboard("entity." .. id .. ".wander_dx")
    local dy = engine.get_blackboard("entity." .. id .. ".wander_dy")
    local timer = engine.get_blackboard("entity." .. id .. ".wander_timer")

    -- Timer-based direction change
    timer = timer - dt
    if timer <= 0 then
        local angle = math.random() * 2 * math.pi
        dx = math.cos(angle)
        dy = math.sin(angle)
        timer = 1.0 + math.random() * 2.0
        engine.set_blackboard("entity." .. id .. ".wander_dx", dx)
        engine.set_blackboard("entity." .. id .. ".wander_dy", dy)
    end
    engine.set_blackboard("entity." .. id .. ".wander_timer", timer)

    -- Reverse direction at world bounds
    local x, y = engine.get_position(id)
    local wx = engine.get_blackboard("world.x")
    local wy = engine.get_blackboard("world.y")
    local ww = engine.get_blackboard("world.width")
    local wh = engine.get_blackboard("world.height")

    if wx ~= nil and wy ~= nil and ww ~= nil and wh ~= nil then
        local ew, eh = engine.get_size(id)
        if x <= wx and dx < 0 then
            dx = math.abs(dx)
        elseif x >= wx + ww - ew and dx > 0 then
            dx = -math.abs(dx)
        end
        if y <= wy and dy < 0 then
            dy = math.abs(dy)
        elseif y >= wy + wh - eh and dy > 0 then
            dy = -math.abs(dy)
        end
        engine.set_blackboard("entity." .. id .. ".wander_dx", dx)
        engine.set_blackboard("entity." .. id .. ".wander_dy", dy)
    end

    -- Set velocity -- MovementSystem applies position + boundary clamping
    engine.set_velocity(id, dx * speed, dy * speed)

    -- Face direction of travel (sprites face right at angle 0)
    engine.set_rotation(id, math.atan(dy, dx))
end

function on_collision(id, other)
    -- Enter scatter state: move away from the colliding duck
    local x, y = engine.get_position(id)
    local ox, oy = engine.get_position(other)
    local ddx = x - ox
    local ddy = y - oy
    local dist = math.sqrt(ddx * ddx + ddy * ddy)

    local sdx, sdy
    if dist > 0.01 then
        sdx = ddx / dist
        sdy = ddy / dist
    else
        sdx = 1
        sdy = 0
    end

    engine.set_blackboard("entity." .. id .. ".fleeing", true)
    engine.set_blackboard("entity." .. id .. ".scatter_timer", 2.0)
    engine.set_blackboard("entity." .. id .. ".scatter_dx", sdx)
    engine.set_blackboard("entity." .. id .. ".scatter_dy", sdy)
end
