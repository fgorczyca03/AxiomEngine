#include "AxiomEngine/assets/AssetManager.h"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#if __has_include(<stb_image.h>)
#define AXIOM_HAS_STB_IMAGE 1
#include <stb_image.h>
#else
#define AXIOM_HAS_STB_IMAGE 0
#endif

#include <cstddef>

namespace axiom::assets {

std::shared_ptr<rendering::Shader> AssetManager::LoadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath) {
    if (const auto it = shaderCache_.find(key); it != shaderCache_.end()) {
        if (auto cached = it->second.lock()) {
            return cached;
        }
    }

    auto shader = std::make_shared<rendering::Shader>();
    if (!shader->LoadFromFiles(vertexPath, fragmentPath)) {
        return {};
    }

    shaderCache_[key] = shader;
    return shader;
}

std::shared_ptr<MeshAsset> AssetManager::LoadGLTFMesh(const std::string& path) {
    if (const auto it = meshCache_.find(path); it != meshCache_.end()) {
        if (auto cached = it->second.lock()) {
            return cached;
        }
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string errors;
    std::string warnings;
    bool loaded = loader.LoadASCIIFromFile(&model, &errors, &warnings, path);
    if (!loaded) {
        loaded = loader.LoadBinaryFromFile(&model, &errors, &warnings, path);
    }

    auto mesh = std::make_shared<MeshAsset>();
    (void)warnings;
    (void)errors;

    if (!loaded || model.meshes.empty() || model.meshes.front().primitives.empty()) {
        meshCache_[path] = mesh;
        return mesh;
    }

    const auto& primitive = model.meshes.front().primitives.front();
    if (const auto posIt = primitive.attributes.find("POSITION"); posIt != primitive.attributes.end()) {
        const auto& accessor = model.accessors[static_cast<std::size_t>(posIt->second)];
        const auto& view = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
        const auto& buffer = model.buffers[static_cast<std::size_t>(view.buffer)];
        const auto* positions = reinterpret_cast<const float*>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
        mesh->vertices.assign(positions, positions + accessor.count * 3U);
    }

    if (primitive.indices >= 0) {
        const auto& accessor = model.accessors[static_cast<std::size_t>(primitive.indices)];
        const auto& view = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
        const auto* indexData = model.buffers[static_cast<std::size_t>(view.buffer)].data.data() + view.byteOffset + accessor.byteOffset;

        mesh->indices.reserve(accessor.count);
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const auto* indices = reinterpret_cast<const std::uint16_t*>(indexData);
            for (std::size_t i = 0; i < accessor.count; ++i) {
                mesh->indices.push_back(indices[i]);
            }
        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const auto* indices = reinterpret_cast<const std::uint32_t*>(indexData);
            mesh->indices.assign(indices, indices + accessor.count);
        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const auto* indices = reinterpret_cast<const std::uint8_t*>(indexData);
            for (std::size_t i = 0; i < accessor.count; ++i) {
                mesh->indices.push_back(indices[i]);
            }
        }
    }

    meshCache_[path] = mesh;
    return mesh;
}

std::shared_ptr<TextureAsset> AssetManager::LoadTexture(const std::string& path) {
    if (const auto it = textureCache_.find(path); it != textureCache_.end()) {
        if (auto cached = it->second.lock()) {
            return cached;
        }
    }

    auto texture = std::make_shared<TextureAsset>();
#if AXIOM_HAS_STB_IMAGE
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    texture->width = width;
    texture->height = height;
    texture->channels = 4;

    if (pixels != nullptr) {
        texture->bytes.assign(pixels, pixels + static_cast<std::size_t>(width * height * 4));
        stbi_image_free(pixels);
    }
#endif

    textureCache_[path] = texture;
    return texture;
}

} // namespace axiom::assets
