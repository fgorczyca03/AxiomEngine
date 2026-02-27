#pragma once

#include <glm/glm.hpp>

namespace axiom::physics {

struct RigidBodyComponent {
    float mass{1.0F};
    glm::vec3 velocity{0.0F};
    glm::vec3 force{0.0F};
    glm::vec3 halfExtent{0.5F};
};

} // namespace axiom::physics
