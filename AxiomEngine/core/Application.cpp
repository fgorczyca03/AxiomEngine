#include "AxiomEngine/core/Application.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <GLFW/glfw3.h>
#include <glm/gtx/quaternion.hpp>
#include <array>
#include <filesystem>
#include <iostream>

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
    const std::string assetRegistryPath = std::string(AXIOM_ASSET_ROOT) + "/assets/registry.axreg";

    std::filesystem::create_directories(std::string(AXIOM_ASSET_ROOT) + "/assets");
    if (!assetRegistry_.LoadFromFile(assetRegistryPath)) {
        assetRegistry_.Register(assets::AssetType::Mesh, prefabPath, 0);
        assetRegistry_.Register(assets::AssetType::Material, std::string(AXIOM_ASSET_ROOT) + "/materials/default.axmat", 0);
        assetRegistry_.SaveToFile(assetRegistryPath);
    }
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
        for (const auto& issue : sceneSerializer_.ValidationIssues()) {
            std::cerr << "Axiom scene validation warning at line " << issue.lineNumber
                      << " entity " << issue.entityIndex << ": " << issue.message << '\n';
        }
        world_.ForEach<scene::TransformComponent>([&](ecs::Entity entity, const scene::TransformComponent&) {
            if (cubeEntity_ == 0) {
                cubeEntity_ = entity;
            }
        });
    }

    if (!scripting_.LoadScript(std::string(AXIOM_ASSET_ROOT) + "/scripts/rotate.lua")) {
        for (const auto& error : scripting_.Errors()) {
            std::cerr << "Axiom script " << error.phase << " error: " << error.message << '\n';
        }
        scripting_.ClearErrors();
    }

    if (!input_.LoadActionMap(actionMapPath)) {
        input_.ClearBindings();
        input_.AddAxisBinding("RotateYaw", GLFW_KEY_A, GLFW_KEY_D, 2.0F);
        input_.AddAxisBinding("RotateYaw", GLFW_KEY_LEFT, GLFW_KEY_RIGHT, 2.0F);
        input_.AddMouseAxisBinding("RotateYaw", 0, 0.01F);
        input_.AddGamepadAxisBinding("RotateYaw", GLFW_GAMEPAD_AXIS_RIGHT_X, 1.0F);
        input_.AddGamepadButtonBinding("RotateYaw", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, 0.5F);
        input_.AddKeyBinding("SensitivityPreset1", GLFW_KEY_1);
        input_.AddGamepadButtonBinding("SensitivityPreset1", GLFW_GAMEPAD_BUTTON_X);
        input_.AddKeyBinding("SensitivityPreset2", GLFW_KEY_2);
        input_.AddGamepadButtonBinding("SensitivityPreset2", GLFW_GAMEPAD_BUTTON_Y);
        input_.AddKeyBinding("SensitivityPreset3", GLFW_KEY_3);
        input_.AddGamepadButtonBinding("SensitivityPreset3", GLFW_GAMEPAD_BUTTON_B);
        input_.AddKeyBinding("RebindLeft", GLFW_KEY_F5);
        input_.AddKeyBinding("RebindAD", GLFW_KEY_F6);
        input_.SetActionDeadzone("RotateYaw", 0.05F);
        input_.SetActionCurveExponent("RotateYaw", 2.0F);
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
    const std::string actionMapPath = std::string(AXIOM_ASSET_ROOT) + "/input/default.axinput";
    const std::array<float, 3> sensitivityPresets{1.0F, 2.0F, 3.5F};
    std::size_t activePreset = 1;

    auto applyPreset = [&](std::size_t presetIndex) {
        activePreset = std::min(presetIndex, sensitivityPresets.size() - 1);
        input_.SetActionCurveExponent("RotateYaw", sensitivityPresets[activePreset]);
        input_.SaveActionMap(actionMapPath);
    };
    applyPreset(activePreset);
    float accumulator = 0.0F;

    while (!renderer_.ShouldClose()) {
        AXIOM_PROFILE_FRAME_MARK();

        renderer_.PollEvents();
        input_.Update();

        input::InputSnapshot snapshot{};
        snapshot.keyState = {{GLFW_KEY_A, renderer_.IsKeyPressed(GLFW_KEY_A)},
                             {GLFW_KEY_D, renderer_.IsKeyPressed(GLFW_KEY_D)},
                             {GLFW_KEY_LEFT, renderer_.IsKeyPressed(GLFW_KEY_LEFT)},
                             {GLFW_KEY_RIGHT, renderer_.IsKeyPressed(GLFW_KEY_RIGHT)},
                             {GLFW_KEY_1, renderer_.IsKeyPressed(GLFW_KEY_1)},
                             {GLFW_KEY_2, renderer_.IsKeyPressed(GLFW_KEY_2)},
                             {GLFW_KEY_3, renderer_.IsKeyPressed(GLFW_KEY_3)},
                             {GLFW_KEY_F5, renderer_.IsKeyPressed(GLFW_KEY_F5)},
                             {GLFW_KEY_F6, renderer_.IsKeyPressed(GLFW_KEY_F6)}};
        snapshot.mouseAxisState = {{0, renderer_.MouseDeltaX()}};
        snapshot.gamepadAxisState = {{GLFW_GAMEPAD_AXIS_RIGHT_X, renderer_.GamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_X)}};
        snapshot.gamepadButtonState = {{GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, renderer_.IsGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER)},
                                       {GLFW_GAMEPAD_BUTTON_X, renderer_.IsGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_X)},
                                       {GLFW_GAMEPAD_BUTTON_Y, renderer_.IsGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_Y)},
                                       {GLFW_GAMEPAD_BUTTON_B, renderer_.IsGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_B)}};
        input_.EvaluateBindings(snapshot);

        if (input_.WasPressed("SensitivityPreset1")) { applyPreset(0); }
        if (input_.WasPressed("SensitivityPreset2")) { applyPreset(1); }
        if (input_.WasPressed("SensitivityPreset3")) { applyPreset(2); }

        if (input_.WasPressed("RebindLeft")) {
            input_.ClearBindings();
            input_.AddAxisBinding("RotateYaw", GLFW_KEY_LEFT, GLFW_KEY_RIGHT, 2.0F);
            input_.AddMouseAxisBinding("RotateYaw", 0, 0.01F);
            input_.AddGamepadAxisBinding("RotateYaw", GLFW_GAMEPAD_AXIS_RIGHT_X, 1.0F);
            input_.AddGamepadButtonBinding("RotateYaw", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, 0.5F);
            input_.AddKeyBinding("SensitivityPreset1", GLFW_KEY_1);
            input_.AddGamepadButtonBinding("SensitivityPreset1", GLFW_GAMEPAD_BUTTON_X);
            input_.AddKeyBinding("SensitivityPreset2", GLFW_KEY_2);
            input_.AddGamepadButtonBinding("SensitivityPreset2", GLFW_GAMEPAD_BUTTON_Y);
            input_.AddKeyBinding("SensitivityPreset3", GLFW_KEY_3);
            input_.AddGamepadButtonBinding("SensitivityPreset3", GLFW_GAMEPAD_BUTTON_B);
            input_.AddKeyBinding("RebindLeft", GLFW_KEY_F5);
            input_.AddKeyBinding("RebindAD", GLFW_KEY_F6);
            input_.SetActionDeadzone("RotateYaw", 0.05F);
            applyPreset(activePreset);
        }

        if (input_.WasPressed("RebindAD")) {
            input_.ClearBindings();
            input_.AddAxisBinding("RotateYaw", GLFW_KEY_A, GLFW_KEY_D, 2.0F);
            input_.AddMouseAxisBinding("RotateYaw", 0, 0.01F);
            input_.AddGamepadAxisBinding("RotateYaw", GLFW_GAMEPAD_AXIS_RIGHT_X, 1.0F);
            input_.AddGamepadButtonBinding("RotateYaw", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, 0.5F);
            input_.AddKeyBinding("SensitivityPreset1", GLFW_KEY_1);
            input_.AddGamepadButtonBinding("SensitivityPreset1", GLFW_GAMEPAD_BUTTON_X);
            input_.AddKeyBinding("SensitivityPreset2", GLFW_KEY_2);
            input_.AddGamepadButtonBinding("SensitivityPreset2", GLFW_GAMEPAD_BUTTON_Y);
            input_.AddKeyBinding("SensitivityPreset3", GLFW_KEY_3);
            input_.AddGamepadButtonBinding("SensitivityPreset3", GLFW_GAMEPAD_BUTTON_B);
            input_.AddKeyBinding("RebindLeft", GLFW_KEY_F5);
            input_.AddKeyBinding("RebindAD", GLFW_KEY_F6);
            input_.SetActionDeadzone("RotateYaw", 0.05F);
            applyPreset(activePreset);
        }
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
            if (scripting_.HasErrors()) {
                for (const auto& error : scripting_.Errors()) {
                    std::cerr << "Axiom script " << error.phase << " error: " << error.message << '\n';
                }
                scripting_.ClearErrors();
            }

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
