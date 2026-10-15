#include <glad/gl.h>  // MUST be first!

#include "AxiomEngine/rendering/Renderer.h"

#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/profiling/Profiling.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
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
    shader_.LoadFromFiles(std::string(AXIOM_ASSET_ROOT) + "/shaders/basic.vert", std::string(AXIOM_ASSET_ROOT) + "/shaders/basic.frag");
    cube_.BuildCube();

    int framebufferWidth = width;
    int framebufferHeight = height;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    if (!ResizeSceneViewport(framebufferWidth, framebufferHeight)) {
        Shutdown();
        return false;
    }

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
        glfwMakeContextCurrent(window_);
        if (sceneDepthBuffer_ != 0U) {
            glDeleteRenderbuffers(1, &sceneDepthBuffer_);
            sceneDepthBuffer_ = 0U;
        }
        if (sceneColorTexture_ != 0U) {
            glDeleteTextures(1, &sceneColorTexture_);
            sceneColorTexture_ = 0U;
        }
        if (sceneFramebuffer_ != 0U) {
            glDeleteFramebuffers(1, &sceneFramebuffer_);
            sceneFramebuffer_ = 0U;
        }
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Renderer::BeginFrame() {
    AXIOM_PROFILE_FUNCTION();
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.1F, 0.1F, 0.15F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderWorld(ecs::ECSWorld& world) {
    AXIOM_PROFILE_FUNCTION();
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer_);
    glViewport(0, 0, sceneWidth_, sceneHeight_);
    glClearColor(0.16F, 0.17F, 0.21F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderContext context{world, camera_, shader_, cube_};
    frameGraph_.Execute(context);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void Renderer::EndFrame() {
    AXIOM_PROFILE_FUNCTION();
    glfwSwapBuffers(window_);
}

bool Renderer::ResizeSceneViewport(int width, int height) {
    if (window_ == nullptr) {
        return false;
    }
    width = std::clamp(width, 1, 4096);
    height = std::clamp(height, 1, 4096);
    if (width == sceneWidth_ && height == sceneHeight_) {
        return true;
    }

    if (sceneFramebuffer_ == 0U) {
        glGenFramebuffers(1, &sceneFramebuffer_);
        glGenTextures(1, &sceneColorTexture_);
        glGenRenderbuffers(1, &sceneDepthBuffer_);
    }

    glBindTexture(GL_TEXTURE_2D, sceneColorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthBuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTexture_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneDepthBuffer_);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Axiom could not create the scene viewport framebuffer (status 0x" << std::hex << status << std::dec
                  << ").\n";
        return false;
    }

    sceneWidth_ = width;
    sceneHeight_ = height;
    camera_.SetPerspective(glm::pi<float>() * 0.25F, static_cast<float>(width) / static_cast<float>(height), 0.1F, 500.0F);
    return true;
}

bool Renderer::ShouldClose() const { return glfwWindowShouldClose(window_) != 0; }

void Renderer::PollEvents() const { glfwPollEvents(); }

GLFWwindow* Renderer::GetWindow() const { return window_; }

unsigned int Renderer::SceneTexture() const { return sceneColorTexture_; }

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
