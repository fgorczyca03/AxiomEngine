local base_speed = 1.0
local t = 0.0

function Update(entity, dt, transform)
    t = t + dt
    local phase = (entity % 4) * 0.5
    transform.y = math.sin(t * 2.0 + phase) * 0.2
    return base_speed * dt
end
