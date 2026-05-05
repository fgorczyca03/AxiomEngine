#include "AxiomEngine/assets/AssetRegistry.h"

#include <fstream>

namespace axiom::assets {

AssetHandle AssetRegistry::Register(AssetType type, const std::string& sourcePath, std::uint64_t sourceHash) {
    if (const auto existing = FindByPath(sourcePath); existing.has_value()) {
        return existing->handle;
    }

    const AssetHandle handle = nextHandle_++;
    AssetMetadata meta{};
    meta.handle = handle;
    meta.type = type;
    meta.sourcePath = sourcePath;
    meta.sourceHash = sourceHash;

    byPath_[sourcePath] = handle;
    byHandle_[handle] = meta;
    return handle;
}

std::optional<AssetMetadata> AssetRegistry::FindByHandle(AssetHandle handle) const {
    const auto it = byHandle_.find(handle);
    if (it == byHandle_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<AssetMetadata> AssetRegistry::FindByPath(const std::string& sourcePath) const {
    const auto pathIt = byPath_.find(sourcePath);
    if (pathIt == byPath_.end()) {
        return std::nullopt;
    }
    return FindByHandle(pathIt->second);
}

bool AssetRegistry::SaveToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "# AxiomAssetRegistry v1\n";
    for (const auto& [handle, meta] : byHandle_) {
        out << handle << "|" << static_cast<unsigned>(meta.type) << "|" << meta.sourceHash << "|" << meta.sourcePath << "\n";
    }

    return true;
}

bool AssetRegistry::LoadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    byHandle_.clear();
    byPath_.clear();
    nextHandle_ = 1;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const std::size_t p0 = line.find('|');
        const std::size_t p1 = line.find('|', p0 + 1);
        const std::size_t p2 = line.find('|', p1 + 1);
        if (p0 == std::string::npos || p1 == std::string::npos || p2 == std::string::npos) {
            continue;
        }

        AssetMetadata meta{};
        meta.handle = static_cast<AssetHandle>(std::stoull(line.substr(0, p0)));
        meta.type = static_cast<AssetType>(std::stoul(line.substr(p0 + 1, p1 - p0 - 1)));
        meta.sourceHash = static_cast<std::uint64_t>(std::stoull(line.substr(p1 + 1, p2 - p1 - 1)));
        meta.sourcePath = line.substr(p2 + 1);

        byHandle_[meta.handle] = meta;
        byPath_[meta.sourcePath] = meta.handle;
        if (meta.handle >= nextHandle_) {
            nextHandle_ = meta.handle + 1;
        }
    }

    return true;
}

} // namespace axiom::assets
