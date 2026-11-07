#pragma once

#include "AxiomEngine/math/Transform.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace axiom::scene {

struct TransformComponent {
    math::Transform local{};
    // Cached parent-composed matrix; systems set dirty whenever they edit the local transform.
    glm::mat4 world{1.0F};
    bool dirty{true};
};

struct SceneNodeComponent {
    // Zero denotes a root; SceneGraph checks that nonzero parent IDs are still alive.
    std::uint32_t parent{0};
};

struct NameComponent {
    std::string value{};
};

struct ScriptComponent {
    std::string scriptPath{};
    std::string lastError{};
    bool enabled{true};
    std::uint64_t instanceId{0};
};

} // namespace axiom::scene
