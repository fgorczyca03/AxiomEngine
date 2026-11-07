-- Each attached object gets a private Lua environment, so this angle is per-object.
local angle = 0.0

function Update(entity, dt, transform)
    -- Update receives a transform table once per fixed simulation step.
    angle = angle + dt
    transform.rotationW = math.cos(angle * 0.5)
    transform.rotationY = math.sin(angle * 0.5)
end
