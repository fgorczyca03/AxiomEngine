#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace axiom::ecs {
class ECSWorld;
}

namespace axiom::rendering {

class Camera;
class Mesh;
class Shader;

struct RenderContext {
    ecs::ECSWorld& world;
    Camera& camera;
    Shader& shader;
    Mesh& mesh;
};

class FrameGraphBuilder {
  public:
    void Read(std::uint32_t resourceId);
    void Write(std::uint32_t resourceId);

    [[nodiscard]] const std::unordered_set<std::uint32_t>& Reads() const;
    [[nodiscard]] const std::unordered_set<std::uint32_t>& Writes() const;

  private:
    std::unordered_set<std::uint32_t> reads_{};
    std::unordered_set<std::uint32_t> writes_{};
};

struct FrameGraphPass {
    std::string name{};
    FrameGraphBuilder builder{};
    std::function<void(RenderContext&)> execute{};
};

class FrameGraph {
  public:
    void Reset();
    FrameGraphPass& AddPass(const std::string& name, std::function<void(FrameGraphBuilder&)> setup, std::function<void(RenderContext&)> execute);
    void Compile();
    void Execute(RenderContext& context);

  private:
    std::vector<FrameGraphPass> passes_{};
    bool compiled_{false};
};

} // namespace axiom::rendering
