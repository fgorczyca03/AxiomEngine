#include "AxiomEngine/scene/SceneGraph.h"

namespace axiom::scene {

void SceneGraph::UpdateTransforms() {
    world_.ForEach<TransformComponent, SceneNodeComponent>(
        [&](ecs::Entity entity, TransformComponent& transform, const SceneNodeComponent& node) {
            if (!transform.dirty) {
                return;
            }
            if (node.parent != 0) {
                if (const auto* parent = world_.GetComponent<TransformComponent>(node.parent)) {
                    transform.world = parent->world * transform.local.LocalMatrix();
                }
            } else {
                transform.world = transform.local.LocalMatrix();
            }
            transform.dirty = false;
            (void)entity;
        });
}

} // namespace axiom::scene
