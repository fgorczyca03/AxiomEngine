#include "AxiomEngine/scene/SceneGraph.h"

#include <functional>
#include <unordered_map>
#include <vector>

namespace axiom::scene {

void SceneGraph::UpdateTransforms() {
    std::vector<ecs::Entity> entities{};
    world_.ForEach<TransformComponent, SceneNodeComponent>(
        [&](ecs::Entity entity, TransformComponent&, const SceneNodeComponent&) { entities.push_back(entity); });

    enum class VisitState : unsigned char { Unvisited, Visiting, Resolved };
    std::unordered_map<ecs::Entity, VisitState> visitStates{};
    visitStates.reserve(entities.size());

    std::function<bool(ecs::Entity)> resolve = [&](ecs::Entity entity) {
        auto* transform = world_.GetComponent<TransformComponent>(entity);
        if (transform == nullptr) {
            return false;
        }

        VisitState& state = visitStates[entity];
        if (state == VisitState::Resolved) {
            return true;
        }
        if (state == VisitState::Visiting) {
            // A stale or cyclic parent link must not recurse forever; the caller will use its local transform.
            return false;
        }
        state = VisitState::Visiting;

        bool hasParentTransform = false;
        const auto* node = world_.GetComponent<SceneNodeComponent>(entity);
        if (node != nullptr && node->parent != 0 && node->parent != entity && world_.IsAlive(node->parent)) {
            if (resolve(node->parent)) {
                if (const auto* parent = world_.GetComponent<TransformComponent>(node->parent)) {
                    transform->world = parent->world * transform->local.LocalMatrix();
                    hasParentTransform = true;
                }
            }
        }
        if (!hasParentTransform) {
            // Missing parents and broken cycles are treated as roots so the scene can still render.
            transform->world = transform->local.LocalMatrix();
        }

        transform->dirty = false;
        state = VisitState::Resolved;
        return true;
    };

    for (const ecs::Entity entity : entities) {
        (void)resolve(entity);
    }
}

} // namespace axiom::scene
