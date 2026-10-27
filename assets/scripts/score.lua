-- score.lua
-- "Calm the Pond" scoring system.
-- Score accumulates while no animals are fleeing.
-- When any animal is disturbed (fleeing flag set), the calm timer resets to zero.

function on_init(id)
    engine.set_blackboard("game.score", 0.0)
    engine.set_blackboard("game.score_text", "Calm: 0.0s")
end

function on_update(id, dt)
    -- Check if any animal is fleeing
    local any_fleeing = false
    local entities = engine.entities_with("Script")
    for _, eid in ipairs(entities) do
        local flag = engine.get_blackboard("entity." .. eid .. ".fleeing")
        if flag == true then
            any_fleeing = true
            break
        end
    end

    -- Update score: count up while calm, reset to zero the moment
    -- anything is disturbed.
    local score = engine.get_blackboard("game.score")
    if score == nil then score = 0.0 end

    if any_fleeing then
        score = 0.0
    else
        score = score + dt
    end
    engine.set_blackboard("game.score", score)

    local count = engine.get_entity_count()
    local text = string.format("Calm: %.1fs | Entities: %d", score, count)
    engine.set_blackboard("game.score_text", text)

    -- Format and store score text
    -- local text = string.format("Calm: %.1fs", score)
    -- engine.set_blackboard("game.score_text", text)
end
