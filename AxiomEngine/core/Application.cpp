#include "AxiomEngine/core/Application.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <glm/gtx/quaternion.hpp>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::core {

Application::Application() : sceneGraph_(world_) {
    world_.RegisterComponent<scene::TransformComponent>();
    world_.RegisterComponent<scene::SceneNodeComponent>();
    world_.RegisterComponent<rendering::MeshComponent>();
    world_.RegisterComponent<physics::RigidBodyComponent>();
}

void Application::InitializeScene() {
    AXIOM_PROFILE_FUNCTION();
    cubeEntity_ = world_.CreateEntity();
    world_.AddComponent(cubeEntity_, scene::TransformComponent{});
    world_.AddComponent(cubeEntity_, scene::SceneNodeComponent{});
    world_.AddComponent(cubeEntity_, rendering::MeshComponent{1, 1});
    world_.AddComponent(cubeEntity_, physics::RigidBodyComponent{});

    scripting_.LoadScript(std::string(AXIOM_ASSET_ROOT) + "/scripts/rotate.lua");
}

int Application::Run() {
    AXIOM_PROFILE_FUNCTION();
    if (!renderer_.Initialize(1280, 720, "Axiom Engine")) {
        return -1;
    }

    editor_.Initialize();
    InitializeScene();

    constexpr float fixedStep = 1.0F / 120.0F;
    float accumulator = 0.0F;

    while (!renderer_.ShouldClose()) {
        AXIOM_PROFILE_FRAME_MARK();

        renderer_.PollEvents();
        input_.Update();
        time_.Tick();
        accumulator += static_cast<float>(time_.DeltaSeconds());

        while (accumulator >= fixedStep) {
            jobs_.Enqueue({"PhysicsStep", JobPriority::High, [&]() { physics_.Step(world_, fixedStep); }});
            jobs_.Flush();

            jobs_.Enqueue({"ScriptUpdate", JobPriority::Normal, [&]() { scripting_.Update(world_, fixedStep); }});
            jobs_.Flush();

            jobs_.Enqueue({
                "TransformResolve",
                JobPriority::Normal,
                [&]() {
                    if (auto* transform = world_.GetComponent<scene::TransformComponent>(cubeEntity_)) {
                        transform->local.rotation =
                            glm::normalize(glm::angleAxis(fixedStep, glm::vec3(0.0F, 1.0F, 0.0F)) * transform->local.rotation);
                        transform->dirty = true;
                    }
                    sceneGraph_.UpdateTransforms();
                },
            });
            jobs_.Flush();

            accumulator -= fixedStep;
        }

        renderer_.BeginFrame();
        renderer_.RenderWorld(world_);

        editor_.BeginFrame();
        editor_.Draw({static_cast<float>(time_.DeltaSeconds())});
        editor_.EndFrame();

        renderer_.EndFrame();
    }

    editor_.Shutdown();
    renderer_.Shutdown();
    return 0;
}

} // namespace axiom::core
