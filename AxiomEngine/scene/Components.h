#pragma once

#include "AxiomEngine/math/Transform.h"

#include <glm/glm.hpp>

namespace axiom::scene {

struct TransformComponent {
    math::Transform local{};
    glm::mat4 world{1.0F};
    bool dirty{true};
};

struct SceneNodeComponent {
    std::uint32_t parent{0};
};

} // namespace axiom::scene
