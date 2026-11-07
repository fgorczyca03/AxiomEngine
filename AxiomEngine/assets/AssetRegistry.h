#pragma once

#include "AxiomEngine/assets/AssetTypes.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace axiom::assets {

class AssetRegistry {
  public:
    // Handles stay stable when the same source path is imported again.
    AssetHandle Register(AssetType type, const std::string& sourcePath, std::uint64_t sourceHash);
    AssetHandle RegisterOrUpdate(
        AssetType type,
        const std::string& sourcePath,
        std::uint64_t sourceHash,
        const std::string& cookedPath,
        std::uint64_t cookedHash);
    std::optional<AssetMetadata> FindByHandle(AssetHandle handle) const;
    std::optional<AssetMetadata> FindByPath(const std::string& sourcePath) const;

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

  private:
    // Keep both lookup directions in sync; serialized scenes store handles while tools browse by path.
    AssetHandle nextHandle_{1};
    std::unordered_map<AssetHandle, AssetMetadata> byHandle_{};
    std::unordered_map<std::string, AssetHandle> byPath_{};
};

} // namespace axiom::assets
