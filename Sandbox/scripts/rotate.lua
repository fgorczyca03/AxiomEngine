local angle = 0.0

function Update(entity, dt, position)
    angle = angle + dt
    position.y = math.sin(angle) * 0.25
    return position
end
