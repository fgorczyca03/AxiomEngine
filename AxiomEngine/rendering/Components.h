#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace axiom::rendering {

struct MaterialComponent {
    glm::vec3 albedo{0.25F, 0.65F, 0.95F};
    float shininess{32.0F};
};

struct MeshComponent {
    std::uint32_t meshHandle{0};
    std::uint32_t materialHandle{0};
};

} // namespace axiom::rendering
