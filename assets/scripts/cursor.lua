-- cursor.lua
-- Tracks the mouse cursor position by reading mouse.x/mouse.y from the Blackboard.
-- The InputSystem writes these values each frame after converting SDL screen
-- coordinates to world coordinates (bottom-left origin, centered).

function on_update(id, dt)
    local mx = engine.get_blackboard("mouse.x")
    local my = engine.get_blackboard("mouse.y")
    if mx ~= nil and my ~= nil then
        engine.set_position(id, mx, my)
    end
end
