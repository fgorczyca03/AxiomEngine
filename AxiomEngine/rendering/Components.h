#pragma once

#include <cstdint>

namespace axiom::rendering {

struct MeshComponent {
    std::uint32_t meshHandle{0};
    std::uint32_t materialHandle{0};
};

} // namespace axiom::rendering
