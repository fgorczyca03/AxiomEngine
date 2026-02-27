#include "AxiomEngine/assets/AssetManager.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#include <stb_image.h>

namespace axiom::assets {

std::shared_ptr<rendering::Shader> AssetManager::LoadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath) {
    if (shaderCache_.contains(key)) {
        if (auto existing = shaderCache_[key].lock()) {
            return existing;
        }
    }

    auto shader = std::make_shared<rendering::Shader>();
    shader->LoadFromFiles(vertexPath, fragmentPath);
    shaderCache_[key] = shader;
    return shader;
}

std::shared_ptr<MeshAsset> AssetManager::LoadGLTFMesh(const std::string& path) {
    if (meshCache_.contains(path)) {
        if (auto existing = meshCache_[path].lock()) {
            return existing;
        }
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string errors;
    std::string warnings;
    loader.LoadASCIIFromFile(&model, &errors, &warnings, path);

    auto asset = std::make_shared<MeshAsset>();
    if (!model.meshes.empty() && !model.meshes.front().primitives.empty()) {
        const auto& primitive = model.meshes.front().primitives.front();
        if (primitive.indices >= 0) {
            const auto& accessor = model.accessors[static_cast<std::size_t>(primitive.indices)];
            const auto& view = model.bufferViews[static_cast<std::size_t>(accessor.bufferView)];
            const auto& buffer = model.buffers[static_cast<std::size_t>(view.buffer)];
            const auto* data = reinterpret_cast<const std::uint16_t*>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
            for (std::size_t i = 0; i < accessor.count; ++i) {
                asset->indices.push_back(static_cast<std::uint32_t>(data[i]));
            }
        }
    }

    meshCache_[path] = asset;
    return asset;
}

std::shared_ptr<TextureAsset> AssetManager::LoadTexture(const std::string& path) {
    if (textureCache_.contains(path)) {
        if (auto existing = textureCache_[path].lock()) {
            return existing;
        }
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* raw = stbi_load(path.c_str(), &width, &height, &channels, 0);

    auto texture = std::make_shared<TextureAsset>();
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    if (raw != nullptr) {
        texture->bytes.assign(raw, raw + static_cast<std::size_t>(width * height * channels));
        stbi_image_free(raw);
    }

    textureCache_[path] = texture;
    return texture;
}

} // namespace axiom::assets
