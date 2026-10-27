-- patrol.lua
-- Horizontal back-and-forth patrol behavior for duck entities.
-- Sets velocity each frame; MovementSystem handles position + boundary clamping.
-- Includes flee behavior: ducks flee from the cursor on collision,
-- resuming patrol once they reach a safe distance.

function on_init(id)
    engine.set_blackboard("entity." .. id .. ".patrol_dir", 1)
    engine.set_blackboard("entity." .. id .. ".patrol_speed", 60)
    engine.set_blackboard("entity." .. id .. ".fleeing", false)
    engine.set_blackboard("entity." .. id .. ".flee_speed", 150)
end

function on_update(id, dt)
    local x, y = engine.get_position(id)
    local fleeing = engine.get_blackboard("entity." .. id .. ".fleeing")

    if fleeing then
        -- Flee: move away from cursor
        local mx = engine.get_blackboard("mouse.x")
        local my = engine.get_blackboard("mouse.y")
        local dx = 1
        local dy = 0
        if mx ~= nil and my ~= nil then
            dx = x - mx
            dy = y - my
            local dist = math.sqrt(dx * dx + dy * dy)

            -- Cancel flee if safe distance reached OR duck hit a world edge
            -- local at_edge = false
            -- local wx = engine.get_blackboard("world.x")
            -- local ww = engine.get_blackboard("world.width")
            -- local wy = engine.get_blackboard("world.y")
            -- local wh = engine.get_blackboard("world.height")
            -- if wx ~= nil then
                -- local ew, eh = engine.get_size(id)
                -- at_edge = (x <= wx) or (x >= wx + ww - ew) or
                          -- (y <= wy) or (y >= wy + wh - eh)
            -- end

            if dist > 150 then -- removed at_edge check

                -- Resume patrol
                engine.set_blackboard("entity." .. id .. ".fleeing", false)
                engine.set_velocity(id, 0, 0)
            else 
                -- Move away from cursor at flee speed
                local flee_speed = engine.get_blackboard("entity." .. id .. ".flee_speed")
                local ndx, ndy
                if dist > 0.01 then
                    ndx = dx / dist
                    ndy = dy / dist
                else
                    ndx = 1 
                    ndy = 0
                end
                engine.set_velocity(id, ndx * flee_speed, ndy * flee_speed)
                engine.set_rotation(id, math.atan(ndy, ndx))
            end
        end
    else
        -- Normal patrol
        local dir = engine.get_blackboard("entity." .. id .. ".patrol_dir")
        local speed = engine.get_blackboard("entity." .. id .. ".patrol_speed")

        -- Reverse at world bounds
        local wx = engine.get_blackboard("world.x")
        local ww = engine.get_blackboard("world.width")
        if wx ~= nil then
            local ew, eh = engine.get_size(id)
            if x >= wx + ww - ew and dir > 0 then
                dir = -1
                engine.set_blackboard("entity." .. id .. ".patrol_dir", dir)
            elseif x <= wx and dir < 0 then
                dir = 1
                engine.set_blackboard("entity." .. id .. ".patrol_dir", dir)
            end
        end

        engine.set_velocity(id, dir * speed, 0)

        -- Sprites face right at angle 0, matching atan2 convention
        if dir > 0 then
            engine.set_rotation(id, 0)
        else
            engine.set_rotation(id, math.pi)
        end
    end
end

function on_collision(id, other_id)
    engine.set_blackboard("entity." .. id .. ".fleeing", true)
end
