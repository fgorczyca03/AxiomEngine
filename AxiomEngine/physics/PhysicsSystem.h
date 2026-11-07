#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

namespace axiom::physics {

// Small fixed-step AABB solver used by the sandbox while collision features grow.
class PhysicsSystem {
  public:
    void Step(ecs::ECSWorld& world, float dt);
};

} // namespace axiom::physics
