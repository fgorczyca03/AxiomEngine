#pragma once

#include "AxiomEngine/rendering/Shader.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace axiom::assets {

struct MeshAsset {
    std::vector<float> vertices;
    std::vector<std::uint32_t> indices;
};

struct TextureAsset {
    int width{0};
    int height{0};
    int channels{0};
    std::vector<std::uint8_t> bytes;
};

class AssetManager {
  public:
    std::shared_ptr<rendering::Shader> LoadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath);
    std::shared_ptr<MeshAsset> LoadGLTFMesh(const std::string& path);
    std::shared_ptr<TextureAsset> LoadTexture(const std::string& path);

  private:
    std::unordered_map<std::string, std::weak_ptr<rendering::Shader>> shaderCache_{};
    std::unordered_map<std::string, std::weak_ptr<MeshAsset>> meshCache_{};
    std::unordered_map<std::string, std::weak_ptr<TextureAsset>> textureCache_{};
};

} // namespace axiom::assets
