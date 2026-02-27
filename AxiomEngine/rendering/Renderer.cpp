#include "AxiomEngine/rendering/Renderer.h"

#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/profiling/Profiling.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/constants.hpp>

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
            context.shader.SetVec3("uLightPos", {2.0F, 3.0F, 2.0F});

            context.world.ForEach<scene::TransformComponent, MeshComponent>([&](ecs::Entity, const scene::TransformComponent& transform, const MeshComponent&) {
                context.shader.SetMat4("uModel", transform.world);
                context.shader.SetMat4("uViewProjection", context.camera.ViewProjection());
                context.mesh.Draw();
            });
        });
    frameGraph_.Compile();
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

} // namespace axiom::rendering
