#include "AxiomEngine/scene/SceneGraph.h"

namespace axiom::scene {

void SceneGraph::Attach(ecs::Entity parent, ecs::Entity child) {
    auto* childNode = world_.GetComponent<SceneNodeComponent>(child);
    auto* parentNode = world_.GetComponent<SceneNodeComponent>(parent);
    if (childNode == nullptr || parentNode == nullptr) {
        return;
    }

    childNode->parent = parent;
    childNode->nextSibling = parentNode->firstChild;
    parentNode->firstChild = child;
    MarkDirty(child);
}

void SceneGraph::MarkDirty(ecs::Entity entity) {
    if (auto* transform = world_.GetComponent<TransformComponent>(entity)) {
        transform->dirty = true;
    }

    if (const auto* node = world_.GetComponent<SceneNodeComponent>(entity)) {
        auto child = node->firstChild;
        while (child != ecs::InvalidEntity) {
            MarkDirty(child);
            const auto* childNode = world_.GetComponent<SceneNodeComponent>(child);
            if (childNode == nullptr) {
                break;
            }
            child = childNode->nextSibling;
        }
    }
}

void SceneGraph::UpdateSubtree(ecs::Entity entity, const glm::mat4& parentWorld) {
    auto* transform = world_.GetComponent<TransformComponent>(entity);
    auto* node = world_.GetComponent<SceneNodeComponent>(entity);
    if (transform == nullptr || node == nullptr) {
        return;
    }

    if (transform->dirty) {
        transform->world = parentWorld * transform->local.LocalMatrix();
        transform->dirty = false;
    }

    auto child = node->firstChild;
    while (child != ecs::InvalidEntity) {
        UpdateSubtree(child, transform->world);
        const auto* childNode = world_.GetComponent<SceneNodeComponent>(child);
        if (childNode == nullptr) {
            break;
        }
        child = childNode->nextSibling;
    }
}

void SceneGraph::UpdateTransforms() {
    world_.ForEach<TransformComponent, SceneNodeComponent>([&](ecs::Entity entity, TransformComponent&, const SceneNodeComponent& node) {
        if (node.parent == ecs::InvalidEntity) {
            UpdateSubtree(entity, glm::mat4{1.0F});
        }
    });
}

} // namespace axiom::scene
