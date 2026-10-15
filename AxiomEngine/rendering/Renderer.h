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
    bool ResizeSceneViewport(int width, int height);

    [[nodiscard]] bool ShouldClose() const;
    void PollEvents() const;
    [[nodiscard]] GLFWwindow* GetWindow() const;
    [[nodiscard]] unsigned int SceneTexture() const;
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
    unsigned int sceneFramebuffer_{0};
    unsigned int sceneColorTexture_{0};
    unsigned int sceneDepthBuffer_{0};
    int sceneWidth_{0};
    int sceneHeight_{0};
    double lastCursorX_{0.0};
    double lastCursorY_{0.0};
    bool cursorCaptured_{false};
};

} // namespace axiom::rendering
