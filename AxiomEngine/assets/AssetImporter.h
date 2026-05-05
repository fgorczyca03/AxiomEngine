#pragma once

#include "AxiomEngine/assets/AssetRegistry.h"

#include <string>

namespace axiom::assets {

class AssetImporter {
  public:
    explicit AssetImporter(AssetRegistry& registry);

    bool Import(AssetType type, const std::string& sourcePath, const std::string& cookedRoot);
    bool ImportDirectory(const std::string& sourceRoot, const std::string& cookedRoot);

  private:
    [[nodiscard]] static AssetType InferTypeFromExtension(const std::string& extension);
    [[nodiscard]] static std::string CookedPathFor(const std::string& sourcePath, const std::string& cookedRoot);
    [[nodiscard]] static std::uint64_t HashFile(const std::string& path);

    AssetRegistry& registry_;
};

} // namespace axiom::assets
