#include "AxiomEngine/assets/AssetRegistry.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace axiom::assets {

namespace {

template <typename T> bool ParseUnsigned(std::string_view text, T& value) {
    if (text.empty()) {
        return false;
    }
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc{} && end == text.data() + text.size();
}

} // namespace

AssetHandle AssetRegistry::Register(AssetType type, const std::string& sourcePath, std::uint64_t sourceHash) {
    if (const auto existing = FindByPath(sourcePath); existing.has_value()) {
        // Reimporting a known path should not invalidate handles already saved in scenes.
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

AssetHandle AssetRegistry::RegisterOrUpdate(
    AssetType type,
    const std::string& sourcePath,
    std::uint64_t sourceHash,
    const std::string& cookedPath,
    std::uint64_t cookedHash) {
    if (const auto existing = FindByPath(sourcePath); existing.has_value()) {
        AssetMetadata updated = *existing;
        updated.type = type;
        updated.sourceHash = sourceHash;
        updated.cookedPath = cookedPath;
        updated.cookedHash = cookedHash;
        byHandle_[updated.handle] = updated;
        byPath_[sourcePath] = updated.handle;
        return updated.handle;
    }

    const AssetHandle handle = Register(type, sourcePath, sourceHash);
    AssetMetadata meta = *FindByHandle(handle);
    meta.cookedPath = cookedPath;
    meta.cookedHash = cookedHash;
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

    out << "# AxiomAssetRegistry v2\n";
    std::vector<AssetHandle> handles{};
    handles.reserve(byHandle_.size());
    for (const auto& [handle, _] : byHandle_) {
        handles.push_back(handle);
    }
    // unordered_map iteration order is not stable, so sort to keep registry files reviewable.
    std::sort(handles.begin(), handles.end());
    for (const AssetHandle handle : handles) {
        const AssetMetadata& meta = byHandle_.at(handle);
        out << handle << "|" << static_cast<unsigned>(meta.type) << "|" << meta.sourceHash << "|" << meta.sourcePath << "|" << meta.cookedHash
            << "|" << meta.cookedPath << "\n";
    }

    return out.good();
}

bool AssetRegistry::LoadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    // Parse into temporary maps; a bad or empty file must not erase the registry already in memory.
    std::unordered_map<AssetHandle, AssetMetadata> loadedByHandle{};
    std::unordered_map<std::string, AssetHandle> loadedByPath{};
    AssetHandle loadedNextHandle = 1;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const std::size_t p0 = line.find('|');
        const std::size_t p1 = line.find('|', p0 + 1);
        const std::size_t p2 = line.find('|', p1 + 1);
        const std::size_t p3 = line.find('|', p2 + 1);
        const std::size_t p4 = line.find('|', p3 + 1);
        if (p0 == std::string::npos || p1 == std::string::npos || p2 == std::string::npos) {
            continue;
        }

        AssetMetadata meta{};
        unsigned int typeValue = 0;
        if (!ParseUnsigned(std::string_view(line).substr(0, p0), meta.handle) || meta.handle == InvalidAssetHandle ||
            !ParseUnsigned(std::string_view(line).substr(p0 + 1, p1 - p0 - 1), typeValue) ||
            typeValue > static_cast<unsigned int>(AssetType::Prefab) ||
            !ParseUnsigned(std::string_view(line).substr(p1 + 1, p2 - p1 - 1), meta.sourceHash)) {
            continue;
        }
        meta.type = static_cast<AssetType>(typeValue);
        if (p3 == std::string::npos && p4 == std::string::npos) {
            meta.sourcePath = line.substr(p2 + 1);
        } else if (p3 != std::string::npos && p4 != std::string::npos) {
            meta.sourcePath = line.substr(p2 + 1, p3 - p2 - 1);
            if (!ParseUnsigned(std::string_view(line).substr(p3 + 1, p4 - p3 - 1), meta.cookedHash)) {
                continue;
            }
            meta.cookedPath = line.substr(p4 + 1);
        } else {
            continue;
        }

        if (meta.sourcePath.empty() || loadedByHandle.contains(meta.handle) || loadedByPath.contains(meta.sourcePath)) {
            continue;
        }

        const AssetHandle handle = meta.handle;
        loadedByPath[meta.sourcePath] = handle;
        loadedByHandle[meta.handle] = std::move(meta);
        if (handle >= loadedNextHandle) {
            loadedNextHandle = handle == std::numeric_limits<AssetHandle>::max() ? handle : handle + 1;
        }
    }

    if (loadedByHandle.empty()) {
        return false;
    }

    byHandle_ = std::move(loadedByHandle);
    byPath_ = std::move(loadedByPath);
    nextHandle_ = loadedNextHandle;
    return true;
}

} // namespace axiom::assets
