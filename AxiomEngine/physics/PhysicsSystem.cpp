#include "AxiomEngine/physics/PhysicsSystem.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/scene/Components.h"

namespace axiom::physics {

void PhysicsSystem::Step(ecs::ECSWorld& world, float dt) {
    world.ForEach<scene::TransformComponent, RigidBodyComponent>([&](ecs::Entity, scene::TransformComponent& transform, RigidBodyComponent& rb) {
        const glm::vec3 acceleration = rb.force / rb.mass;
        rb.velocity += acceleration * dt;
        transform.local.translation += rb.velocity * dt;
        transform.dirty = true;
        rb.force = glm::vec3(0.0F);

        if (transform.local.translation.y < -2.0F) {
            transform.local.translation.y = -2.0F;
            rb.velocity.y *= -0.4F;
        }
    });
}

} // namespace axiom::physics
