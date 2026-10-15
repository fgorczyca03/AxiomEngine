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
    [[nodiscard]] glm::vec2 MouseDelta();
    void SetCursorCaptured(bool captured);
    [[nodiscard]] bool IsCursorCaptured() const;
    void MoveCamera(float forward, float right, float up, float deltaSeconds);
    void RotateCamera(float deltaX, float deltaY);
    [[nodiscard]] bool IsGamepadButtonPressed(int button) const;
    [[nodiscard]] float GamepadAxis(int axis) const;

  private:
    GLFWwindow* window_{nullptr};
    Camera camera_{};
    Shader shader_{};
    Mesh cube_{};
    FrameGraph frameGraph_{};
    double lastCursorX_{0.0};
    double lastCursorY_{0.0};
    bool cursorCaptured_{false};
};

} // namespace axiom::rendering
