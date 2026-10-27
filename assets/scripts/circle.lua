
function on_init (id)
    local x,y = engine.get_position(id)
    engine.set_blackboard("entity." .. id .. ".center_x", x)
    engine.set_blackboard("entity." .. id .. ".center_y", y)
    engine.set_blackboard("entity." .. id .. ".circle_angle", 0)
    engine.set_blackboard("entity." .. id .. ".circle_radius", 100)
    engine.set_blackboard("entity." .. id .. ".circle_speed", -2.0)
    
    
end

function on_update(id, dt)
    local centerX = engine.get_blackboard("entity." .. id .. ".center_x")
    local centerY = engine.get_blackboard("entity." .. id .. ".center_y")
    local angle = engine.get_blackboard("entity." .. id .. ".circle_angle")
    local radius = engine.get_blackboard("entity." .. id .. ".circle_radius")
    local speed = engine.get_blackboard("entity." .. id .. ".circle_speed")

    angle = angle + speed * dt

    local x = centerX + radius * math.cos(angle)
    local y = centerY + radius * math.sin(angle)

    engine.set_position(id, x, y)
    engine.set_rotation(id, angle + math.pi / 2)

    engine.set_blackboard("entity." .. id .. ".circle_angle", angle)
end