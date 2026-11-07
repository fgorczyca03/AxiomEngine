#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/scene/Components.h"

namespace axiom::scene {

// Resolves parent-first world matrices after physics and scripts edit local transforms.
class SceneGraph {
  public:
    explicit SceneGraph(ecs::ECSWorld& world) : world_(world) {}

    void UpdateTransforms();

  private:
    ecs::ECSWorld& world_;
};

} // namespace axiom::scene
