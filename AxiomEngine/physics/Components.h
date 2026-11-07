#pragma once

#include <glm/glm.hpp>

namespace axiom::physics {

struct RigidBodyComponent {
    // A non-positive mass is treated as immovable by the simple overlap solver.
    float mass{1.0F};
    glm::vec3 velocity{0.0F};
    glm::vec3 force{0.0F};
    // Collision bounds are axis-aligned half-sizes, independent of mesh geometry.
    glm::vec3 halfExtent{0.5F};
};

} // namespace axiom::physics
