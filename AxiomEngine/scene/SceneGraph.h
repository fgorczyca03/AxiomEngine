#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/scene/Components.h"

#include <unordered_map>
#include <vector>

namespace axiom::scene {

class SceneGraph {
  public:
    explicit SceneGraph(ecs::ECSWorld& world) : world_(world) {}

    void UpdateTransforms();

  private:
    enum class VisitState : unsigned char { Unvisited, Visiting, Resolved };

    struct VisitInfo {
        VisitState state{VisitState::Unvisited};
        bool worldChanged{false};
    };

    struct ResolveResult {
        bool resolved{false};
        bool worldChanged{false};
    };

    [[nodiscard]] ResolveResult ResolveTransform(ecs::Entity entity);

    ecs::ECSWorld& world_;
    std::vector<ecs::Entity> entities_{};
    std::unordered_map<ecs::Entity, VisitInfo> visitStates_{};
};

} // namespace axiom::scene
