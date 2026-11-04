local angle = 0.0

function Update(entity, dt, transform)
    angle = angle + dt
    transform.rotationW = math.cos(angle * 0.5)
    transform.rotationY = math.sin(angle * 0.5)
end
