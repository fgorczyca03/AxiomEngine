#include "AxiomEngine/physics/PhysicsSystem.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <algorithm>
#include <vector>

namespace axiom::physics {

namespace {

constexpr float FloorY = -2.0F;
constexpr float Restitution = 0.4F;
constexpr float MinCollisionDepth = 0.0001F;

struct BodyRef {
    ecs::Entity entity{};
    scene::TransformComponent* transform{nullptr};
    RigidBodyComponent* rigidBody{nullptr};
};

struct AxisOverlap {
    glm::vec3 normal{0.0F};
    float depth{0.0F};
};

float InverseMass(const RigidBodyComponent& body) {
    return body.mass > 0.0F ? 1.0F / body.mass : 0.0F;
}

AxisOverlap CalculateMinimumOverlap(const BodyRef& a, const BodyRef& b) {
    const glm::vec3 delta = b.transform->local.translation - a.transform->local.translation;
    const glm::vec3 overlap = a.rigidBody->halfExtent + b.rigidBody->halfExtent - glm::abs(delta);

    AxisOverlap result{};
    result.depth = overlap.x;
    result.normal = {delta.x < 0.0F ? -1.0F : 1.0F, 0.0F, 0.0F};

    if (overlap.y < result.depth) {
        result.depth = overlap.y;
        result.normal = {0.0F, delta.y < 0.0F ? -1.0F : 1.0F, 0.0F};
    }
    if (overlap.z < result.depth) {
        result.depth = overlap.z;
        result.normal = {0.0F, 0.0F, delta.z < 0.0F ? -1.0F : 1.0F};
    }

    return result;
}

bool AabbOverlaps(const BodyRef& a, const BodyRef& b) {
    const glm::vec3 delta = glm::abs(b.transform->local.translation - a.transform->local.translation);
    const glm::vec3 allowed = a.rigidBody->halfExtent + b.rigidBody->halfExtent;
    return delta.x < allowed.x && delta.y < allowed.y && delta.z < allowed.z;
}

void ResolvePairCollision(BodyRef& a, BodyRef& b) {
    if (!AabbOverlaps(a, b)) {
        return;
    }

    const AxisOverlap overlap = CalculateMinimumOverlap(a, b);
    if (overlap.depth <= MinCollisionDepth) {
        return;
    }

    const float inverseMassA = InverseMass(*a.rigidBody);
    const float inverseMassB = InverseMass(*b.rigidBody);
    const float inverseMassTotal = inverseMassA + inverseMassB;
    if (inverseMassTotal <= 0.0F) {
        return;
    }

    const glm::vec3 correction = overlap.normal * overlap.depth;
    a.transform->local.translation -= correction * (inverseMassA / inverseMassTotal);
    b.transform->local.translation += correction * (inverseMassB / inverseMassTotal);
    a.transform->dirty = true;
    b.transform->dirty = true;

    const glm::vec3 relativeVelocity = b.rigidBody->velocity - a.rigidBody->velocity;
    const float velocityAlongNormal = glm::dot(relativeVelocity, overlap.normal);
    if (velocityAlongNormal > 0.0F) {
        return;
    }

    const float impulseMagnitude = -(1.0F + Restitution) * velocityAlongNormal / inverseMassTotal;
    const glm::vec3 impulse = impulseMagnitude * overlap.normal;
    a.rigidBody->velocity -= impulse * inverseMassA;
    b.rigidBody->velocity += impulse * inverseMassB;
}

} // namespace

void PhysicsSystem::Step(ecs::ECSWorld& world, float dt) {
    std::vector<BodyRef> bodies{};
    world.ForEach<scene::TransformComponent, RigidBodyComponent>(
        [&](ecs::Entity entity, scene::TransformComponent& transform, RigidBodyComponent& rb) {
            bodies.push_back({entity, &transform, &rb});
        });

    std::sort(bodies.begin(), bodies.end(), [](const BodyRef& lhs, const BodyRef& rhs) { return lhs.entity < rhs.entity; });

    for (BodyRef& body : bodies) {
        const float inverseMass = InverseMass(*body.rigidBody);
        if (inverseMass > 0.0F) {
            const glm::vec3 acceleration = body.rigidBody->force * inverseMass;
            body.rigidBody->velocity += acceleration * dt;
            body.transform->local.translation += body.rigidBody->velocity * dt;
            body.transform->dirty = true;
        }
        body.rigidBody->force = glm::vec3(0.0F);

        const float floorCenter = FloorY + body.rigidBody->halfExtent.y;
        if (body.transform->local.translation.y < floorCenter) {
            body.transform->local.translation.y = floorCenter;
            body.rigidBody->velocity.y *= -Restitution;
            body.transform->dirty = true;
        }
    }

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            ResolvePairCollision(bodies[i], bodies[j]);
        }
    }
}

} // namespace axiom::physics
