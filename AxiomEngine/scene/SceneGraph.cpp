#include "AxiomEngine/scene/SceneGraph.h"

namespace axiom::scene {

void SceneGraph::UpdateTransforms() {
    entities_.clear();
    world_.ForEach<TransformComponent, SceneNodeComponent>(
        [&](ecs::Entity entity, TransformComponent&, const SceneNodeComponent&) { entities_.push_back(entity); });

    for (auto& [_, visit] : visitStates_) {
        visit.state = VisitState::Unvisited;
        visit.worldChanged = false;
    }

    for (const ecs::Entity entity : entities_) {
        (void)ResolveTransform(entity);
    }
}

SceneGraph::ResolveResult SceneGraph::ResolveTransform(ecs::Entity entity) {
    auto* transform = world_.GetComponent<TransformComponent>(entity);
    if (transform == nullptr) {
        return {};
    }

    const auto existing = visitStates_.find(entity);
    if (existing != visitStates_.end()) {
        if (existing->second.state == VisitState::Resolved) {
            return {true, existing->second.worldChanged};
        }
        if (state == VisitState::Visiting) {
            // A stale or cyclic parent link must not recurse forever; the caller will use its local transform.
            return false;
        if (existing->second.state == VisitState::Visiting) {
            return {};
        }
    }
    visitStates_[entity].state = VisitState::Visiting;

    bool hasParentTransform = false;
    bool parentChanged = false;
    bool forceLocalUpdate = transform->dirty;
    const auto* node = world_.GetComponent<SceneNodeComponent>(entity);
    if (node != nullptr && node->parent != 0U && node->parent != entity) {
        if (!world_.IsAlive(node->parent)) {
            forceLocalUpdate = true;
        } else {
            const ResolveResult parentResult = ResolveTransform(node->parent);
            if (parentResult.resolved) {
                if (world_.GetComponent<TransformComponent>(node->parent) != nullptr) {
                    hasParentTransform = true;
                    parentChanged = parentResult.worldChanged;
                }
            } else {
                // A missing or cyclic parent falls back to local space, matching the
                // previous resolver behavior while still avoiding work on normal roots.
                forceLocalUpdate = true;
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
    }

    const bool worldChanged = forceLocalUpdate || parentChanged;
    if (worldChanged) {
        const glm::mat4 localMatrix = transform->local.LocalMatrix();
        if (hasParentTransform) {
            const auto* parent = world_.GetComponent<TransformComponent>(node->parent);
            transform->world = parent->world * localMatrix;
        } else {
            transform->world = localMatrix;
        }
    }

    transform->dirty = false;
    VisitInfo& visit = visitStates_[entity];
    visit.state = VisitState::Resolved;
    visit.worldChanged = worldChanged;
    return {true, worldChanged};
}

} // namespace axiom::scene
