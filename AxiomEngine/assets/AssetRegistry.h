#pragma once

#include "AxiomEngine/assets/AssetTypes.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace axiom::assets {

class AssetRegistry {
  public:
    AssetHandle Register(AssetType type, const std::string& sourcePath, std::uint64_t sourceHash);
    std::optional<AssetMetadata> FindByHandle(AssetHandle handle) const;
    std::optional<AssetMetadata> FindByPath(const std::string& sourcePath) const;

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

  private:
    AssetHandle nextHandle_{1};
    std::unordered_map<AssetHandle, AssetMetadata> byHandle_{};
    std::unordered_map<std::string, AssetHandle> byPath_{};
};

} // namespace axiom::assets
