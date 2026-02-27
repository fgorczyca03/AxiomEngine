#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/math/Transform.h"

#include <glm/glm.hpp>

namespace axiom::scene {

struct TransformComponent {
    math::Transform local{};
    glm::mat4 world{1.0F};
    bool dirty{true};
};

struct SceneNodeComponent {
    ecs::Entity parent{ecs::InvalidEntity};
    ecs::Entity firstChild{ecs::InvalidEntity};
    ecs::Entity nextSibling{ecs::InvalidEntity};
};

} // namespace axiom::scene
