#pragma once

#include <cstdint>
#include <string>

namespace axiom::assets {

using AssetHandle = std::uint64_t;

// Zero stays reserved so an unset component handle never names a real asset.
constexpr AssetHandle InvalidAssetHandle = 0;

enum class AssetType : std::uint8_t {
    Unknown = 0,
    Mesh,
    Texture,
    Material,
    Script,
    Shader,
    Scene,
    Prefab,
};

struct AssetMetadata {
    AssetHandle handle{InvalidAssetHandle};
    AssetType type{AssetType::Unknown};
    std::string sourcePath{};
    // These hashes let the importer tell whether source or cooked data changed.
    std::uint64_t sourceHash{0};
    std::string cookedPath{};
    std::uint64_t cookedHash{0};
};

} // namespace axiom::assets
