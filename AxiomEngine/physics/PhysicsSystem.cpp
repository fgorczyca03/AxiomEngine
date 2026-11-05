#include "AxiomEngine/physics/PhysicsSystem.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <algorithm>

namespace axiom::physics {

namespace {

using BodyRef = detail::PhysicsBodyRef;

constexpr float FloorY = -2.0F;
constexpr float Restitution = 0.4F;
constexpr float MinCollisionDepth = 0.0001F;

struct AxisOverlap {
    glm::vec3 normal{0.0F};
    float depth{0.0F};
};

float InverseMass(const RigidBodyComponent& body) {
    return body.mass > 0.0F ? 1.0F / body.mass : 0.0F;
}

bool CalculateMinimumOverlap(const BodyRef& a, const BodyRef& b, AxisOverlap& result) {
    const glm::vec3 delta = b.transform->local.translation - a.transform->local.translation;
    const glm::vec3 overlap = a.rigidBody->halfExtent + b.rigidBody->halfExtent - glm::abs(delta);
    if (!(overlap.x > 0.0F && overlap.y > 0.0F && overlap.z > 0.0F)) {
        return false;
    }

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

    return result.depth > MinCollisionDepth;
}

void ResolvePairCollision(BodyRef& a, BodyRef& b) {
    const float inverseMassA = InverseMass(*a.rigidBody);
    const float inverseMassB = InverseMass(*b.rigidBody);
    const float inverseMassTotal = inverseMassA + inverseMassB;
    if (inverseMassTotal <= 0.0F) {
        return;
    }

    AxisOverlap overlap{};
    if (!CalculateMinimumOverlap(a, b, overlap)) {
        return;
    }

    const glm::vec3 correction = overlap.normal * overlap.depth;
    const glm::vec3 correctionA = correction * (inverseMassA / inverseMassTotal);
    if (correctionA.x != 0.0F || correctionA.y != 0.0F || correctionA.z != 0.0F) {
        a.transform->local.translation -= correctionA;
        a.transform->dirty = true;
    }
    const glm::vec3 correctionB = correction * (inverseMassB / inverseMassTotal);
    if (correctionB.x != 0.0F || correctionB.y != 0.0F || correctionB.z != 0.0F) {
        b.transform->local.translation += correctionB;
        b.transform->dirty = true;
    }

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
    bodies_.clear();
    world.ForEach<scene::TransformComponent, RigidBodyComponent>(
        [&](ecs::Entity entity, scene::TransformComponent& transform, RigidBodyComponent& rb) {
            bodies_.push_back({entity, &transform, &rb});
        });

    std::sort(bodies_.begin(), bodies_.end(), [](const BodyRef& lhs, const BodyRef& rhs) {
        return lhs.entity < rhs.entity;
    });

    for (BodyRef& body : bodies_) {
        const float inverseMass = InverseMass(*body.rigidBody);
        if (inverseMass > 0.0F) {
            const glm::vec3 acceleration = body.rigidBody->force * inverseMass;
            body.rigidBody->velocity += acceleration * dt;
            const glm::vec3 displacement = body.rigidBody->velocity * dt;
            if (displacement.x != 0.0F || displacement.y != 0.0F || displacement.z != 0.0F) {
                body.transform->local.translation += displacement;
                body.transform->dirty = true;
            }
        }
        body.rigidBody->force = glm::vec3(0.0F);

        const float floorCenter = FloorY + body.rigidBody->halfExtent.y;
        if (body.transform->local.translation.y < floorCenter) {
            body.transform->local.translation.y = floorCenter;
            if (body.rigidBody->velocity.y < 0.0F) {
                body.rigidBody->velocity.y = -body.rigidBody->velocity.y * Restitution;
            }
            body.transform->dirty = true;
        }
    }

    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies_.size(); ++j) {
            ResolvePairCollision(bodies_[i], bodies_[j]);
        }
    }
}

} // namespace axiom::physics
