#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Camera.h"
#include "AxiomEngine/rendering/FrameGraph.h"
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
    [[nodiscard]] bool IsKeyPressed(int keyCode) const;
    [[nodiscard]] float MouseDeltaX();
    [[nodiscard]] bool IsGamepadButtonPressed(int button) const;
    [[nodiscard]] float GamepadAxis(int axis) const;

  private:
    GLFWwindow* window_{nullptr};
    Camera camera_{};
    Shader shader_{};
    Mesh cube_{};
    FrameGraph frameGraph_{};
    double lastCursorX_{0.0};
};

} // namespace axiom::rendering
