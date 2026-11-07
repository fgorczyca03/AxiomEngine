#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

#include <vector>

namespace axiom::scene {
struct TransformComponent;
}

namespace axiom::physics {

// Small fixed-step AABB solver used by the sandbox while collision features grow.
class PhysicsSystem {
  public:
    void Step(ecs::ECSWorld& world, float dt);

  private:
    std::vector<detail::PhysicsBodyRef> bodies_{};
};

} // namespace axiom::physics
