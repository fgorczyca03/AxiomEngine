#include "AxiomEngine/assets/AssetImporter.h"

#include <filesystem>
#include <fstream>

namespace axiom::assets {

namespace {
constexpr std::string_view kCookedExtension = ".axcooked";
}

AssetImporter::AssetImporter(AssetRegistry& registry) : registry_(registry) {}

bool AssetImporter::Import(AssetType type, const std::string& sourcePath, const std::string& cookedRoot) {
    namespace fs = std::filesystem;

    if (!fs::exists(sourcePath) || fs::is_directory(sourcePath)) {
        return false;
    }

    const std::string cookedPath = CookedPathFor(sourcePath, cookedRoot);
    fs::create_directories(fs::path(cookedPath).parent_path());

    const std::uint64_t sourceHash = HashFile(sourcePath);
    const std::optional<AssetMetadata> existing = registry_.FindByPath(sourcePath);
    if (existing.has_value() && existing->sourceHash == sourceHash && existing->cookedPath == cookedPath && fs::exists(cookedPath)) {
        return true;
    }

    fs::copy_file(sourcePath, cookedPath, fs::copy_options::overwrite_existing);
    const std::uint64_t cookedHash = HashFile(cookedPath);
    registry_.RegisterOrUpdate(type, sourcePath, sourceHash, cookedPath, cookedHash);
    return true;
}

bool AssetImporter::ImportDirectory(const std::string& sourceRoot, const std::string& cookedRoot) {
    namespace fs = std::filesystem;

    if (!fs::exists(sourceRoot) || !fs::is_directory(sourceRoot)) {
        return false;
    }

    bool importedAny = false;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(sourceRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const AssetType type = InferTypeFromExtension(entry.path().extension().string());
        if (type == AssetType::Unknown) {
            continue;
        }

        if (Import(type, entry.path().string(), cookedRoot)) {
            importedAny = true;
        }
    }

    return importedAny;
}

AssetType AssetImporter::InferTypeFromExtension(const std::string& extension) {
    if (extension == ".gltf" || extension == ".glb") {
        return AssetType::Mesh;
    }
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
        return AssetType::Texture;
    }
    if (extension == ".lua") {
        return AssetType::Script;
    }
    if (extension == ".vert" || extension == ".frag") {
        return AssetType::Shader;
    }
    if (extension == ".axscene") {
        return AssetType::Scene;
    }
    if (extension == ".axprefab") {
        return AssetType::Prefab;
    }
    return AssetType::Unknown;
}

std::string AssetImporter::CookedPathFor(const std::string& sourcePath, const std::string& cookedRoot) {
    namespace fs = std::filesystem;
    const fs::path source{sourcePath};
    return (fs::path(cookedRoot) / source.filename()).replace_extension(kCookedExtension).string();
}

std::uint64_t AssetImporter::HashFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return 0;
    }

    constexpr std::uint64_t fnvOffset = 1469598103934665603ULL;
    constexpr std::uint64_t fnvPrime = 1099511628211ULL;

    std::uint64_t hash = fnvOffset;
    char byte = 0;
    while (in.get(byte)) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= fnvPrime;
    }
    return hash;
}

} // namespace axiom::assets
