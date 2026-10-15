#include <glad/gl.h>  // MUST be first!

#include "AxiomEngine/rendering/Renderer.h"

#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/profiling/Profiling.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <array>
#include <iostream>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::rendering {

namespace {

void GlfwErrorCallback(int errorCode, const char* description) {
    std::cerr << "Axiom GLFW error (" << errorCode << "): "
              << (description != nullptr ? description : "unknown error") << '\n';
}

} // namespace

bool Renderer::Initialize(int width, int height, const char* title) {
    AXIOM_PROFILE_FUNCTION();
    glfwSetErrorCallback(GlfwErrorCallback);
    if (glfwInit() == 0) {
        std::cerr << "Axiom GLFW initialization failed.\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        std::cerr << "Axiom could not create an OpenGL 4.5 core window. "
                     "Check the GLFW error above and confirm the graphics driver supports OpenGL 4.5.\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "Axiom could not load OpenGL entry points for the active context. "
                     "The renderer cannot safely continue.\n";
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    camera_.SetPerspective(glm::pi<float>() * 0.25F, static_cast<float>(width) / static_cast<float>(height), 0.1F, 500.0F);
    shader_.LoadFromFiles(std::string(AXIOM_ASSET_ROOT) + "/shaders/basic.vert", std::string(AXIOM_ASSET_ROOT) + "/shaders/basic.frag");
    cube_.BuildCube();

    frameGraph_.Reset();
    frameGraph_.AddPass(
        "ForwardOpaque",
        [](FrameGraphBuilder& builder) {
            builder.Write(1);
            builder.Write(2);
        },
        [&](RenderContext& context) {
            context.shader.Bind();
            context.shader.SetVec3("uLightPos", {2.0F, 4.0F, 2.0F});
            context.shader.SetVec3("uLightColor", {1.0F, 0.95F, 0.85F});
            context.shader.SetVec3("uViewPos", context.camera.Position());
            context.shader.SetFloat("uAmbientStrength", 0.18F);
            context.shader.SetFloat("uShadowStrength", 0.35F);

            constexpr std::array<glm::vec3, 4> materialPalette{
                glm::vec3{0.25F, 0.65F, 0.95F},
                glm::vec3{0.95F, 0.42F, 0.28F},
                glm::vec3{0.35F, 0.9F, 0.55F},
                glm::vec3{0.85F, 0.75F, 0.35F},
            };

            context.world.ForEach<scene::TransformComponent, MeshComponent>(
                [&](ecs::Entity, const scene::TransformComponent& transform, const MeshComponent& mesh) {
                    const glm::vec3 baseColor = materialPalette[mesh.materialHandle % materialPalette.size()];
                    context.shader.SetMat4("uModel", transform.world);
                    context.shader.SetMat4("uViewProjection", context.camera.ViewProjection());
                    context.shader.SetVec3("uBaseColor", baseColor);
                    context.shader.SetFloat("uRoughness", mesh.materialHandle == 0U ? 0.65F : 0.35F);
                    context.mesh.Draw();
                });
        });
    frameGraph_.Compile();
    glfwGetCursorPos(window_, &lastCursorX_, &lastCursorY_);
    return true;
}

void Renderer::Shutdown() {
    AXIOM_PROFILE_FUNCTION();
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

void Renderer::BeginFrame() {
    AXIOM_PROFILE_FUNCTION();
    glClearColor(0.1F, 0.1F, 0.15F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderWorld(ecs::ECSWorld& world) {
    AXIOM_PROFILE_FUNCTION();
    RenderContext context{world, camera_, shader_, cube_};
    frameGraph_.Execute(context);
}

void Renderer::EndFrame() {
    AXIOM_PROFILE_FUNCTION();
    glfwSwapBuffers(window_);
}

bool Renderer::ShouldClose() const { return glfwWindowShouldClose(window_) != 0; }

void Renderer::PollEvents() const { glfwPollEvents(); }

bool Renderer::IsKeyPressed(int keyCode) const {
    return glfwGetKey(window_, keyCode) == GLFW_PRESS;
}


glm::vec2 Renderer::MouseDelta() {
    if (window_ == nullptr) {
        return glm::vec2(0.0F);
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    const glm::vec2 delta{static_cast<float>(x - lastCursorX_), static_cast<float>(y - lastCursorY_)};
    lastCursorX_ = x;
    lastCursorY_ = y;
    return delta;
}

void Renderer::SetCursorCaptured(bool captured) {
    if (window_ == nullptr || cursorCaptured_ == captured) {
        return;
    }

    cursorCaptured_ = captured;
    glfwSetInputMode(window_, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    glfwGetCursorPos(window_, &lastCursorX_, &lastCursorY_);
}

bool Renderer::IsCursorCaptured() const { return cursorCaptured_; }

void Renderer::MoveCamera(float forward, float right, float up, float deltaSeconds) {
    camera_.Move(forward, right, up, deltaSeconds);
}

void Renderer::RotateCamera(float deltaX, float deltaY) { camera_.Rotate(deltaX, deltaY); }

bool Renderer::IsGamepadButtonPressed(int button) const {
    if (button < 0 || button >= GLFW_GAMEPAD_BUTTON_LAST + 1) {
        return false;
    }

    GLFWgamepadstate state{};
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) == GLFW_FALSE) {
        return false;
    }
    return state.buttons[button] == GLFW_PRESS;
}

float Renderer::GamepadAxis(int axis) const {
    if (axis < 0 || axis >= GLFW_GAMEPAD_AXIS_LAST + 1) {
        return 0.0F;
    }

    GLFWgamepadstate state{};
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) == GLFW_FALSE) {
        return 0.0F;
    }
    return state.axes[axis];
}

} // namespace axiom::rendering
