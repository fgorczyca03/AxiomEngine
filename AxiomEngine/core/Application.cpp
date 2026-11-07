#include "AxiomEngine/core/Application.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <filesystem>
#include <iostream>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::core {

Application::Application() : sceneGraph_(world_) {
    world_.RegisterComponent<scene::TransformComponent>();
    world_.RegisterComponent<scene::SceneNodeComponent>();
    world_.RegisterComponent<scene::NameComponent>();
    world_.RegisterComponent<scene::ScriptComponent>();
    world_.RegisterComponent<rendering::MeshComponent>();
    world_.RegisterComponent<physics::RigidBodyComponent>();
    scripting_.SetAssetRoot(AXIOM_ASSET_ROOT);
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
    const bool sceneFileExists = std::filesystem::exists(scenePath);
    const bool sceneLoaded = sceneSerializer_.Load(world_, scenePath);
    for (const auto& issue : sceneSerializer_.ValidationIssues()) {
        std::cerr << "Axiom scene validation warning at line " << issue.lineNumber
                  << " entity " << issue.entityIndex << ": " << issue.message << '\n';
    }

    if (!sceneLoaded) {
        if (sceneFileExists) {
            std::cerr << "Axiom scene could not be loaded; using the fallback scene without overwriting the existing file: "
                      << scenePath << '\n';
        }

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

        if (prefabEntity.has_value() && world_.GetComponent<scene::NameComponent>(*prefabEntity) == nullptr) {
            const auto* mesh = world_.GetComponent<rendering::MeshComponent>(*prefabEntity);
            const std::string name = mesh != nullptr ? std::string(rendering::PrimitiveShapeName(mesh->primitive)) : "Entity";
            world_.AddComponent(*prefabEntity, scene::NameComponent{name + " 1"});
        }

        if (prefabEntity.has_value() && !sceneFileExists) {
            sceneSerializer_.Save(world_, scenePath);
        }
    }

    bool actionMapChanged = false;
    if (!input_.LoadActionMap(actionMapPath)) {
        input_.ClearBindings();
        actionMapChanged = true;
    }
    const auto hasAction = [this](const std::string& actionName) {
        const auto names = input_.ActionNames();
        return std::find(names.begin(), names.end(), actionName) != names.end();
    };
    if (!hasAction("CameraForward")) {
        input_.AddAxisBinding("CameraForward", GLFW_KEY_S, GLFW_KEY_W);
        actionMapChanged = true;
    }
    if (!hasAction("CameraRight")) {
        input_.AddAxisBinding("CameraRight", GLFW_KEY_A, GLFW_KEY_D);
        actionMapChanged = true;
    }
    if (!hasAction("CameraVertical")) {
        input_.AddAxisBinding("CameraVertical", GLFW_KEY_Q, GLFW_KEY_E);
        actionMapChanged = true;
    }
    if (!hasAction("ToggleCameraLook")) {
        input_.AddKeyBinding("ToggleCameraLook", GLFW_KEY_F1);
        actionMapChanged = true;
    }
    if (actionMapChanged) {
        input_.SaveActionMap(actionMapPath);
    }
}

int Application::Run() {
    AXIOM_PROFILE_FUNCTION();
    if (!renderer_.Initialize(1280, 720, "Axiom Engine")) {
        std::cerr << "Axiom renderer startup failed; see the GLFW/OpenGL diagnostics above.\n";
        return -1;
    }

    if (!editor_.Initialize(renderer_.GetWindow(), AXIOM_ASSET_ROOT)) {
        std::cerr << "Axiom editor initialization failed.\n";
        renderer_.Shutdown();
        return -1;
    }
    InitializeScene();

    constexpr float fixedStep = 1.0F / 120.0F;
    float accumulator = 0.0F;

    while (!renderer_.ShouldClose()) {
        AXIOM_PROFILE_FRAME_MARK();

        renderer_.PollEvents();
        input_.Update();

        input::InputSnapshot snapshot{};
        snapshot.keyState = {{GLFW_KEY_A, renderer_.IsKeyPressed(GLFW_KEY_A)},
                             {GLFW_KEY_D, renderer_.IsKeyPressed(GLFW_KEY_D)},
                             {GLFW_KEY_W, renderer_.IsKeyPressed(GLFW_KEY_W)},
                             {GLFW_KEY_S, renderer_.IsKeyPressed(GLFW_KEY_S)},
                             {GLFW_KEY_Q, renderer_.IsKeyPressed(GLFW_KEY_Q)},
                             {GLFW_KEY_E, renderer_.IsKeyPressed(GLFW_KEY_E)},
                             {GLFW_KEY_F1, renderer_.IsKeyPressed(GLFW_KEY_F1)}};
        glm::vec2 mouseDelta = renderer_.MouseDelta();
        if (editor_.WantsKeyboardCapture()) {
            snapshot.keyState.clear();
        }
        if (editor_.WantsMouseCapture() && !renderer_.IsCursorCaptured()) {
            snapshot.mouseAxisState.clear();
            mouseDelta = glm::vec2(0.0F);
        }
        const bool replayingInput = inputReplay_.Mode() == input::InputReplayMode::Playback;
        if (!replayingInput) {
            input_.EvaluateBindings(snapshot);
        }

        if (!replayingInput && input_.WasPressed("ToggleCameraLook")) {
            renderer_.SetCursorCaptured(!renderer_.IsCursorCaptured());
            mouseDelta = glm::vec2(0.0F);
        }

        time_.Tick();
        const float cameraDeltaSeconds =
            static_cast<float>(std::clamp(time_.DeltaSeconds(), 0.0, 0.1));
        if (!replayingInput && renderer_.IsCursorCaptured()) {
            renderer_.MoveCamera(input_.Value("CameraForward"),
                                 input_.Value("CameraRight"),
                                 input_.Value("CameraVertical"),
                                 cameraDeltaSeconds);
            renderer_.RotateCamera(mouseDelta.x, mouseDelta.y);
        }
        accumulator += static_cast<float>(time_.DeltaSeconds());

        while (accumulator >= fixedStep) {
            if (inputReplay_.Mode() == input::InputReplayMode::Playback &&
                !inputReplay_.ApplyNextFixedStep(input_)) {
                const std::string replayError = inputReplay_.LastError();
                inputReplay_.StopPlayback();
                if (!replayError.empty()) {
                    std::cerr << "Axiom input replay stopped: " << replayError << '\n';
                }
                input_.EvaluateBindings(snapshot);
            }
            if (inputReplay_.Mode() == input::InputReplayMode::Recording &&
                !inputReplay_.RecordFixedStep(input_)) {
                std::cerr << "Axiom input replay stopped: " << inputReplay_.LastError() << '\n';
            }

            const std::size_t physicsJob =
                jobs_.Enqueue({"PhysicsStep", JobPriority::High, [&]() { physics_.Step(world_, fixedStep); }});

            const std::size_t scriptJob = jobs_.Enqueue(
                {"ScriptUpdate", JobPriority::Normal, [&]() { scripting_.Update(world_, fixedStep); }, {physicsJob}});

            jobs_.Enqueue({
                "TransformResolve",
                JobPriority::Normal,
                [&]() { sceneGraph_.UpdateTransforms(); },
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

        if (!renderer_.ResizeSceneViewport(editor_.SceneViewportWidth(), editor_.SceneViewportHeight())) {
            std::cerr << "Axiom scene viewport resize failed.\n";
            break;
        }
        renderer_.BeginFrame();
        renderer_.RenderWorld(world_);

        editor_.BeginFrame();
        editor_.Draw({static_cast<float>(time_.DeltaSeconds())}, world_, sceneSerializer_, renderer_.SceneTexture(),
                     renderer_, input_, inputReplay_);
        editor_.EndFrame();

        renderer_.EndFrame();
    }

    editor_.Shutdown();
    renderer_.Shutdown();
    return 0;
}

} // namespace axiom::core
