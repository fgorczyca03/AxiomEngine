#include "AxiomEngine/physics/PhysicsSystem.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <cmath>
#include <vector>

namespace axiom::physics {

namespace {
struct BodyView {
    ecs::Entity entity{ecs::InvalidEntity};
    scene::TransformComponent* transform{nullptr};
    RigidBodyComponent* body{nullptr};
};

bool IntersectsAABB(const BodyView& a, const BodyView& b) {
    const auto ap = a.transform->local.translation;
    const auto bp = b.transform->local.translation;
    const auto ah = a.body->halfExtent;
    const auto bh = b.body->halfExtent;

    return (std::abs(ap.x - bp.x) <= (ah.x + bh.x)) && (std::abs(ap.y - bp.y) <= (ah.y + bh.y)) && (std::abs(ap.z - bp.z) <= (ah.z + bh.z));
}
} // namespace

void PhysicsSystem::Step(ecs::ECSWorld& world, float dt) {
    std::vector<BodyView> bodies;

    world.ForEach<scene::TransformComponent, RigidBodyComponent>([&](ecs::Entity entity, scene::TransformComponent& transform, RigidBodyComponent& body) {
        const float safeMass = body.mass > 0.0001F ? body.mass : 1.0F;
        const glm::vec3 acceleration = body.force / safeMass;
        body.velocity += acceleration * dt;
        transform.local.translation += body.velocity * dt;
        body.force = glm::vec3{0.0F};
        transform.dirty = true;

        if (transform.local.translation.y < -2.0F) {
            transform.local.translation.y = -2.0F;
            body.velocity.y *= -0.25F;
        }

        bodies.push_back(BodyView{entity, &transform, &body});
    });

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            if (!IntersectsAABB(bodies[i], bodies[j])) {
                continue;
            }

            const auto correction = (bodies[j].transform->local.translation - bodies[i].transform->local.translation) * 0.5F;
            bodies[i].transform->local.translation -= correction;
            bodies[j].transform->local.translation += correction;
            bodies[i].body->velocity *= -0.2F;
            bodies[j].body->velocity *= -0.2F;
            bodies[i].transform->dirty = true;
            bodies[j].transform->dirty = true;
        }
    }
}

} // namespace axiom::physics
