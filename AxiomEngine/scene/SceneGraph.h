#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/scene/Components.h"

#include <glm/glm.hpp>

namespace axiom::scene {

class SceneGraph {
  public:
    explicit SceneGraph(ecs::ECSWorld& world) : world_(world) {}

    void Attach(ecs::Entity parent, ecs::Entity child);
    void MarkDirty(ecs::Entity entity);
    void UpdateTransforms();

  private:
    void UpdateSubtree(ecs::Entity entity, const glm::mat4& parentWorld);

    ecs::ECSWorld& world_;
};

} // namespace axiom::scene
