#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Camera.h"
#include "AxiomEngine/rendering/Mesh.h"
#include "AxiomEngine/rendering/Shader.h"

struct GLFWwindow;

namespace axiom::rendering {

class Renderer {
  public:
    bool Initialize(int width, int height, const char* title);
    void Shutdown();

    void BeginFrame();
    void RenderWorld(ecs::ECSWorld& world);
    void EndFrame();

    [[nodiscard]] bool ShouldClose() const;
    void PollEvents() const;

  private:
    GLFWwindow* window_{nullptr};
    Camera camera_{};
    Shader shader_{};
    Mesh cube_{};
};

} // namespace axiom::rendering
