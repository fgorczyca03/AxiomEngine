#pragma once

#include <cstdint>
#include <array>
#include <cstddef>
#include <string_view>

namespace axiom::rendering {

enum class PrimitiveShape : std::uint8_t {
    Cube = 0,
    Sphere,
    Circle,
    Pyramid,
    Cone,
    Cylinder,
    Bean,
    Count,
};

inline constexpr std::size_t PrimitiveShapeCount = static_cast<std::size_t>(PrimitiveShape::Count);
inline constexpr std::array<std::string_view, PrimitiveShapeCount> PrimitiveShapeNames{
    "Cube", "Sphere", "Circle", "Pyramid", "Cone", "Cylinder", "Bean",
};

[[nodiscard]] constexpr bool IsValidPrimitiveShape(std::uint32_t value) {
    return value < PrimitiveShapeCount;
}

[[nodiscard]] constexpr std::string_view PrimitiveShapeName(PrimitiveShape shape) {
    const auto index = static_cast<std::size_t>(shape);
    return index < PrimitiveShapeNames.size() ? PrimitiveShapeNames[index] : PrimitiveShapeNames[0];
}

struct MeshComponent {
    std::uint32_t meshHandle{0};
    std::uint32_t materialHandle{0};
    PrimitiveShape primitive{PrimitiveShape::Cube};
};

} // namespace axiom::rendering
