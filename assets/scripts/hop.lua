-- hop.lua
-- Sit-then-hop behavior for frog entities.
-- Uses set_position for discrete hops (not continuous movement).
-- MovementSystem boundary clamping keeps frogs in bounds between hops.
-- Velocity is zeroed so frogs stay put until the next hop.

function on_init(id)
    math.randomseed(id)
    engine.set_blackboard("entity." .. id .. ".hop_timer", 2.0 + math.random() * 3.0)
    engine.set_blackboard("entity." .. id .. ".hop_distance", 80)
    -- Zero velocity so frog sits still between hops
    engine.set_velocity(id, 0, 0)
end

function on_update(id, dt)
    local timer = engine.get_blackboard("entity." .. id .. ".hop_timer")
    timer = timer - dt

    if timer <= 0 then
        local x, y = engine.get_position(id)
        local dist = engine.get_blackboard("entity." .. id .. ".hop_distance")
        local angle = math.random() * 2 * math.pi
        local new_x = x + math.cos(angle) * dist
        local new_y = y + math.sin(angle) * dist

        -- Clamp to world bounds (accounting for entity size)
        local wx = engine.get_blackboard("world.x")
        local wy = engine.get_blackboard("world.y")
        local ww = engine.get_blackboard("world.width")
        local wh = engine.get_blackboard("world.height")

        if wx ~= nil and wy ~= nil and ww ~= nil and wh ~= nil then
            local ew, eh = engine.get_size(id)
            new_x = math.max(wx, math.min(wx + ww - ew, new_x))
            new_y = math.max(wy, math.min(wy + wh - eh, new_y))
        end

        engine.set_position(id, new_x, new_y)

        -- Face the direction of the hop (sprites face right at angle 0)
        engine.set_rotation(id, angle)

        timer = 2.0 + math.random() * 3.0
    end

    engine.set_blackboard("entity." .. id .. ".hop_timer", timer)
end
