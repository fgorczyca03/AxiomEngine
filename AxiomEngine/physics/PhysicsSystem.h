#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

namespace axiom::physics {

class PhysicsSystem {
  public:
    void Step(ecs::ECSWorld& world, float dt);
};

} // namespace axiom::physics
