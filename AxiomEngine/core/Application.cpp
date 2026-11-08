#include "AxiomEngine/core/Application.h"

#include "AxiomEngine/core/PlaySession.h"
#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::core {

Application::Application() : sceneGraph_(world_) {
    // Component types must be registered before the first entity or system query uses them.
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
        // Keep a broken existing scene intact so it can be repaired; only a missing file gets initialized on disk.
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

    // Load the user's bindings first, then add only the camera actions the editor depends on.
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
    PlaySession play;
    auto& replay = inputReplay_;
    bool replayFinished = false;
    input::InputSystem simulationInput;
    std::string runtimeStatus;
    const std::string replayPath = std::string(AXIOM_ASSET_ROOT) + "/replays/latest.axreplay";
    time_.Tick();

    while (!renderer_.ShouldClose()) {
        AXIOM_PROFILE_FRAME_MARK();

        renderer_.PollEvents();
        time_.Tick();
        bool resetAccumulator = false;
        const auto startPlay = [&]() {
            if (!play.Start(world_)) return;
            scripting_.Reset();
            simulationInput = input_;
            simulationInput.ApplyActionStates({});
            resetAccumulator = true;
            replayFinished = false;
        };
        using editor::RuntimeCommand;
        switch (editor_.ConsumeRuntimeCommand()) {
            case RuntimeCommand::Play:
                startPlay();
                runtimeStatus.clear();
                break;
            case RuntimeCommand::Record:
                if (!play.IsPlaying() && replay.StartRecording(replayPath, fixedStep)) {
                    startPlay();
                    runtimeStatus = "Recording fixed-step input. Stop to finish.";
                } else {
                    runtimeStatus = replay.LastError();
                }
                break;
            case RuntimeCommand::Replay:
                if (!play.IsPlaying() && replay.StartPlayback(replayPath, fixedStep)) {
                    startPlay();
                    runtimeStatus = "Replaying input from the current edit scene.";
                } else {
                    runtimeStatus = replay.LastError();
                }
                break;
            case RuntimeCommand::Pause: play.SetPaused(true); resetAccumulator = true; break;
            case RuntimeCommand::Resume: play.SetPaused(false); resetAccumulator = true; break;
            case RuntimeCommand::Step: play.RequestStep(); break;
            case RuntimeCommand::Stop:
                replay.StopRecording();
                replay.StopPlayback();
                if (play.Stop(world_)) scripting_.Reset();
                renderer_.SetCursorCaptured(false);
                resetAccumulator = true;
                replayFinished = false;
                runtimeStatus = "Stopped. Restored the edit scene.";
                break;
            case RuntimeCommand::None: break;
        }
        if (resetAccumulator) accumulator = 0.0F;
        input_.Update();

        input::InputSnapshot snapshot{};
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
            snapshot.SetKeyState(key, renderer_.IsKeyPressed(key));
        }
        glm::vec2 mouseDelta = renderer_.MouseDelta();
        snapshot.mouseAxisState[0] = mouseDelta.x;
        snapshot.mouseAxisState[1] = mouseDelta.y;
        for (int axis = 0; axis <= GLFW_GAMEPAD_AXIS_LAST; ++axis) snapshot.gamepadAxisState[axis] = renderer_.GamepadAxis(axis);
        for (int button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; ++button) {
            snapshot.gamepadButtonState[button] = renderer_.IsGamepadButtonPressed(button);
        }
        if (editor_.WantsKeyboardCapture()) {
            snapshot.ClearKeyState();
        }
        if (editor_.WantsMouseCapture() && !renderer_.IsCursorCaptured()) {
            snapshot.mouseAxisState.clear();
            mouseDelta = glm::vec2(0.0F);
        }
        input_.EvaluateBindings(snapshot);

        if (input_.WasPressed("ToggleCameraLook")) {
            renderer_.SetCursorCaptured(!renderer_.IsCursorCaptured());
            mouseDelta = glm::vec2(0.0F);
        }

        const float cameraDeltaSeconds =
            static_cast<float>(std::clamp(time_.DeltaSeconds(), 0.0, 0.1));
        if (renderer_.IsCursorCaptured()) {
            renderer_.MoveCamera(input_.Value("CameraForward"),
                                 input_.Value("CameraRight"),
                                 input_.Value("CameraVertical"),
                                 cameraDeltaSeconds);
            renderer_.RotateCamera(mouseDelta.x, mouseDelta.y);
        }
        if (play.IsPlaying() && !play.IsPaused() && !resetAccumulator) {
            accumulator += static_cast<float>(std::clamp(time_.DeltaSeconds(), 0.0, 0.25));
        } else if (play.IsPaused()) {
            accumulator = play.ShouldStep() ? fixedStep : 0.0F;
        }

        const auto simulationStart = std::chrono::steady_clock::now();
        std::size_t simulationSteps = 0;
        while (play.ShouldStep() && accumulator >= fixedStep) {
            if (replay.Mode() == input::InputReplayMode::Playback) {
                if (!replay.ApplyNextFixedStep(simulationInput)) {
                    play.SetPaused(true);
                    replayFinished = true;
                    runtimeStatus = replay.LastError().empty()
                        ? "Replay complete. Stop to restore the edit scene." : replay.LastError();
                    accumulator = 0.0F;
                    break;
                }
            } else {
                simulationInput.Update();
                simulationInput.EvaluateBindings(snapshot);
                if (replay.Mode() == input::InputReplayMode::Recording && !replay.RecordFixedStep(simulationInput)) {
                    runtimeStatus = replay.LastError();
                    play.SetPaused(true);
                    break;
                }
            }
            // Physics and scripts finish before world transforms are published for rendering.
            const std::size_t physicsJob =
                jobs_.Enqueue({"PhysicsStep", JobPriority::High, [&]() { physics_.Step(world_, fixedStep); }});

            const std::size_t scriptJob = jobs_.Enqueue({
                "ScriptUpdate", JobPriority::Normal,
                [&]() { scripting_.Update(world_, fixedStep, &simulationInput); }, {physicsJob}});

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
            play.DidStep();
            ++simulationSteps;
        }
        const float simulationMilliseconds = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - simulationStart).count();
        // Edit-mode gizmos must still update world matrices while simulation is stopped.
        sceneGraph_.UpdateTransforms();

        if (!renderer_.ResizeSceneViewport(editor_.SceneViewportWidth(), editor_.SceneViewportHeight())) {
            std::cerr << "Axiom scene viewport resize failed.\n";
            break;
        }
        renderer_.BeginFrame();
        renderer_.RenderWorld(world_);

        editor_.BeginFrame();
        editor_.Draw({.deltaSeconds = static_cast<float>(time_.DeltaSeconds()),
                      .simulationMilliseconds = simulationMilliseconds,
                      .simulationSteps = simulationSteps,
                      .tick = play.Tick(),
                      .playing = play.IsPlaying(),
                      .paused = play.IsPaused(),
                      .recording = (replay.Mode() == input::InputReplayMode::Recording),
                      .replayFinished = replayFinished,
                      .runtimeStatus = runtimeStatus}, world_, sceneSerializer_, renderer_.SceneTexture(),
                     renderer_, play.IsPlaying() ? simulationInput : input_, replay);
        editor_.EndFrame();

        renderer_.EndFrame();
    }

    replay.StopRecording();
    replay.StopPlayback();
    if (play.Stop(world_)) scripting_.Reset();
    editor_.Shutdown();
    renderer_.Shutdown();
    return 0;
}

} // namespace axiom::core
