#include "AxiomEngine/core/Application.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <GLFW/glfw3.h>
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

    const std::string scenePath = std::string(AXIOM_ASSET_ROOT) + "/scenes/default.axscene";
    const std::string prefabPath = std::string(AXIOM_ASSET_ROOT) + "/prefabs/cube.axprefab";
    const std::string actionMapPath = std::string(AXIOM_ASSET_ROOT) + "/input/default.axinput";
    if (!sceneSerializer_.Load(world_, scenePath)) {
        auto prefabEntity = prefab_.InstantiateFromFile(world_, prefabPath);
        if (!prefabEntity.has_value()) {
            scene::PrefabData prefabData{};
            prefabData.transform = scene::TransformComponent{};
            prefabData.node = scene::SceneNodeComponent{};
            prefabData.mesh = rendering::MeshComponent{1, 1};
            prefabData.rigidBody = physics::RigidBodyComponent{};
            prefab_.Save(prefabData, prefabPath);
            prefabEntity = prefab_.Instantiate(world_, prefabData);
        }

        if (prefabEntity.has_value()) {
            cubeEntity_ = *prefabEntity;
        }
        sceneSerializer_.Save(world_, scenePath);
    } else {
        world_.ForEach<scene::TransformComponent>([&](ecs::Entity entity, const scene::TransformComponent&) {
            if (cubeEntity_ == 0) {
                cubeEntity_ = entity;
            }
        });
    }

    scripting_.LoadScript(std::string(AXIOM_ASSET_ROOT) + "/scripts/rotate.lua");

    if (!input_.LoadActionMap(actionMapPath)) {
        input_.ClearBindings();
        input_.AddAxisBinding("RotateYaw", GLFW_KEY_A, GLFW_KEY_D, 2.0F);
        input_.SetActionDeadzone("RotateYaw", 0.05F);
        input_.SetActionCurveExponent("RotateYaw", 1.5F);
        input_.SaveActionMap(actionMapPath);
    }
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
        input_.EvaluateBindings({{GLFW_KEY_A, renderer_.IsKeyPressed(GLFW_KEY_A)}, {GLFW_KEY_D, renderer_.IsKeyPressed(GLFW_KEY_D)}});
        time_.Tick();
        accumulator += static_cast<float>(time_.DeltaSeconds());

        while (accumulator >= fixedStep) {
            const std::size_t physicsJob =
                jobs_.Enqueue({"PhysicsStep", JobPriority::High, [&]() { physics_.Step(world_, fixedStep); }});

            const std::size_t scriptJob = jobs_.Enqueue(
                {"ScriptUpdate", JobPriority::Normal, [&]() { scripting_.Update(world_, fixedStep); }, {physicsJob}});

            jobs_.Enqueue({
                "TransformResolve",
                JobPriority::Normal,
                [&]() {
                    if (auto* transform = world_.GetComponent<scene::TransformComponent>(cubeEntity_)) {
                        const float yawSpeed = input_.Value("RotateYaw");
                        transform->local.rotation =
                            glm::normalize(glm::angleAxis(fixedStep * yawSpeed, glm::vec3(0.0F, 1.0F, 0.0F)) * transform->local.rotation);
                        transform->dirty = true;
                    }
                    sceneGraph_.UpdateTransforms();
                },
                {scriptJob},
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
