#include "AxiomEngine/rendering/Renderer.h"

#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/profiling/Profiling.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/constants.hpp>
#include <array>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::rendering {

bool Renderer::Initialize(int width, int height, const char* title) {
    AXIOM_PROFILE_FUNCTION();
    if (glfwInit() == 0) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        return false;
    }

    glfwMakeContextCurrent(window_);
    gladLoadGL(glfwGetProcAddress);

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
    glfwGetCursorPos(window_, &lastCursorX_, nullptr);
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


float Renderer::MouseDeltaX() {
    double x = 0.0;
    glfwGetCursorPos(window_, &x, nullptr);
    const float delta = static_cast<float>(x - lastCursorX_);
    lastCursorX_ = x;
    return delta;
}

bool Renderer::IsGamepadButtonPressed(int button) const {
    GLFWgamepadstate state{};
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) == GLFW_FALSE) {
        return false;
    }
    return state.buttons[button] == GLFW_PRESS;
}

float Renderer::GamepadAxis(int axis) const {
    GLFWgamepadstate state{};
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) == GLFW_FALSE) {
        return 0.0F;
    }
    return state.axes[axis];
}

} // namespace axiom::rendering
