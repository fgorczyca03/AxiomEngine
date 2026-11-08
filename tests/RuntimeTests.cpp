#include "AxiomEngine/core/PlaySession.h"
#include "AxiomEngine/input/InputReplay.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/scene/SceneGraph.h"
#include "AxiomEngine/scripting/ScriptSystem.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct TemporaryDirectory {
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("axiom-runtime-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    TemporaryDirectory() { std::filesystem::create_directories(path / "scripts"); }
    ~TemporaryDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};

void TestPlaySession() {
    using namespace axiom;
    ecs::ECSWorld world;
    world.RegisterComponent<scene::NameComponent>();
    world.RegisterComponent<scene::SceneNodeComponent>();
    const auto parent = world.CreateEntity();
    const auto child = world.CreateEntity();
    world.AddComponent(parent, scene::NameComponent{"Unsaved name"});
    world.AddComponent(child, scene::SceneNodeComponent{parent});
    const auto reusable = world.CreateEntity();
    world.DestroyEntity(reusable);

    core::PlaySession play;
    Check(!play.ShouldStep() && !play.Stop(world), "Edit mode must not simulate");
    Check(play.Start(world) && !play.Start(world), "Play must snapshot only once");
    world.GetComponent<scene::NameComponent>(parent)->value = "Runtime name";
    const auto spawned = world.CreateEntity();
    world.DestroyEntity(child);
    world.RegisterComponent<int>();
    world.AddComponent(spawned, 42);
    play.SetPaused(true);
    Check(!play.ShouldStep(), "Pause must suspend simulation");
    play.RequestStep();
    Check(play.ShouldStep(), "Step must allow one tick while paused");
    play.DidStep();
    Check(!play.ShouldStep() && play.Tick() == 1, "Step must consume exactly one tick");
    play.SetPaused(false);
    Check(play.ShouldStep(), "Resume must allow simulation");
    Check(play.Stop(world), "Stop must restore the edit world");
    Check(world.GetComponent<scene::NameComponent>(parent)->value == "Unsaved name", "Stop lost unsaved component data");
    Check(world.IsAlive(child) && world.GetComponent<scene::SceneNodeComponent>(child)->parent == parent,
          "Stop lost entity IDs or hierarchy");
    Check(!world.IsAlive(spawned) && world.CreateEntity() == reusable, "Stop lost entity allocator state");
    Check(play.Tick() == 0 && !play.IsPlaying(), "Stop must reset play state");
}

void TestInputReplay(const std::filesystem::path& directory) {
    using namespace axiom::input;
    constexpr float step = 1.0F / 120.0F;
    const auto file = (directory / "input.axreplay").string();
    InputSystem input;
    input.AddKeyBinding("Jump", 32);
    input.AddGamepadAxisBinding("Move", 0);
    input.SetActionDeadzone("Move", 0.2F);
    input.SetActionCurveExponent("Move", 2.0F);
    InputReplay replay;
    Check(!replay.StartRecording(file, 0.0F) && replay.StartRecording(file, step), "Recording must validate its time step");
    InputSnapshot snapshot;
    const std::array<bool, 4> keys{true, true, false, false};
    for (const bool pressed : keys) {
        input.Update();
        snapshot.SetKeyState(32, pressed);
        snapshot.gamepadAxisState[0] = -0.73F;
        input.EvaluateBindings(snapshot);
        Check(replay.RecordFixedStep(input), "Failed to record fixed tick");
    }
    const float expectedAxis = input.Value("Move");
    Check(replay.StopRecording(), "Failed to finish recording");
    Check(!replay.StartPlayback(file, step * 2.0F) && replay.StartPlayback(file, step),
          "Mismatched fixed step must be rejected");
    InputSystem playback;
    playback.SetActionState("LiveOnly", true);
    playback.SetActionState("Jump", true);
    for (std::size_t tick = 0; tick < keys.size(); ++tick) {
        Check(replay.ApplyNextFixedStep(playback), "Replay ended before its last tick");
        Check(playback.IsPressed("Jump") == keys[tick], "Replay changed button state");
        Check(playback.WasPressed("Jump") == (tick == 0), "Replay repeated or suppressed a press edge");
        Check(playback.WasReleased("Jump") == (tick == 2), "Replay lost a release edge");
        Check(playback.Value("Move") == expectedAxis, "Replay did not round-trip the exact shaped float");
        Check(!playback.IsPressed("LiveOnly"), "Live input leaked into replay");
    }
    Check(!replay.ApplyNextFixedStep(playback) && replay.FrameIndex() == 4 && replay.LastError().empty(),
          "Replay must stop cleanly at EOF without adding a tick");
    replay.StopPlayback();

    const std::array<std::string, 9> invalid{
        "WRONG\n",
        "AXIOM_INPUT_REPLAY_V2\n-1\n",
        "AXIOM_INPUT_REPLAY_V2\n0\n",
        "AXIOM_INPUT_REPLAY_V1\nFRAME 4097\n",
        "AXIOM_INPUT_REPLAY_V1\nFRAME 1\n",
        "AXIOM_INPUT_REPLAY_V1\nFRAME 2\n\"Jump\" 1 1 0 1\n\"Jump\" 0 0 1 0\n",
        "AXIOM_INPUT_REPLAY_V1\nFRAME 1\n\"Jump\" 0 0 0 1\n",
        "AXIOM_INPUT_REPLAY_V1\nFRAME 1\n\"Move\" 1 0 0 2\n",
        "AXIOM_INPUT_REPLAY_V1\nTRAILING\n",
    };
    for (const auto& contents : invalid) {
        const auto badFile = (directory / "bad.axreplay").string();
        { std::ofstream out(badFile); out << contents; }
        playback.SetActionState("Unchanged", true);
        if (replay.StartPlayback(badFile)) Check(!replay.ApplyNextFixedStep(playback), "Malformed capture was accepted");
        Check(!replay.LastError().empty() && playback.IsPressed("Unchanged"), "Malformed capture partially changed input");
        replay.StopPlayback();
    }
    const auto legacy = (directory / "legacy.axreplay").string();
    { std::ofstream out(legacy); out << "AXIOM_INPUT_REPLAY_V1\nFRAME 1\n\"Jump\" 1 1 0 1\n"; }
    Check(replay.StartPlayback(legacy) && replay.ApplyNextFixedStep(playback) && playback.WasPressed("Jump"),
          "Legacy V1 recordings must remain readable");
    replay.StopPlayback();
}

void TestScriptReplay(const std::filesystem::path& directory) {
    using namespace axiom;
    constexpr float step = 1.0F / 120.0F;
    {
        std::ofstream out(directory / "scripts" / "input.lua");
        out << "local updates = 0\n"
               "function Update(entity, dt, transform, input)\n"
               "  updates = updates + 1\n"
               "  transform.x = transform.x + input.Move.value * dt\n"
               "  if input.Jump.justPressed then transform.y = transform.y + 1 end\n"
               "  transform.z = updates\n"
               "end\n";
    }
    ecs::ECSWorld world;
    world.RegisterComponent<scene::TransformComponent>();
    world.RegisterComponent<scene::SceneNodeComponent>();
    world.RegisterComponent<scene::ScriptComponent>();
    const auto entity = world.CreateEntity();
    world.AddComponent(entity, scene::TransformComponent{});
    world.AddComponent(entity, scene::SceneNodeComponent{});
    world.AddComponent(entity, scene::ScriptComponent{"scripts/input.lua"});
    scripting::ScriptSystem scripts;
    scripts.SetAssetRoot(directory);
    scene::SceneGraph graph(world);
    core::PlaySession play;
    input::InputSystem input;
    input.AddAxisBinding("Move", 65, 68);
    input.AddKeyBinding("Jump", 32);
    input::InputReplay replay;
    const auto replayFile = (directory / "script.axreplay").string();
    Check(play.Start(world) && replay.StartRecording(replayFile, step), "Failed to start script recording");
    const std::array<int, 5> directions{68, 68, 65, 65, 68};
    for (std::size_t tick = 0; tick < directions.size(); ++tick) {
        input::InputSnapshot snapshot;
        snapshot.SetKeyState(directions[tick], true);
        snapshot.SetKeyState(32, tick < 2 || tick == 4);
        input.Update();
        input.EvaluateBindings(snapshot);
        Check(replay.RecordFixedStep(input), "Failed to record script input");
        scripts.Update(world, step, &input);
        graph.UpdateTransforms();
        play.DidStep();
        Check(!scripts.HasErrors(), "Script could not read simulation input");
    }
    const auto expected = world.GetComponent<scene::TransformComponent>(entity)->local.translation;
    Check(expected.y == 2.0F && expected.z == 5.0F, "Script did not receive tick-specific input edges");
    Check(replay.StopRecording(), "Failed to finish script recording");
    Check(play.Stop(world), "Failed to restore recorded world");

    // Different numbers of fixed ticks per render frame must give the same result.
    for (const std::size_t batchSize : {1U, 3U, 8U}) {
        scripts.Reset();
        Check(play.Start(world) && replay.StartPlayback(replayFile, step), "Failed to start replay");
        while (replay.FrameIndex() < directions.size()) {
            for (std::size_t tick = 0; tick < batchSize && replay.ApplyNextFixedStep(input); ++tick) {
                scripts.Update(world, step, &input);
                graph.UpdateTransforms();
                play.DidStep();
            }
        }
        Check(!scripts.HasErrors(), "Script failed during replay");
        Check(world.GetComponent<scene::TransformComponent>(entity)->local.translation == expected,
              "Replay result changed with render batching or leaked script state");
        Check(play.Tick() == directions.size(), "Replay ran an incorrect number of ticks");
        replay.StopPlayback();
        Check(play.Stop(world), "Failed to stop replay");
        Check(world.GetComponent<scene::TransformComponent>(entity)->local.translation == glm::vec3(0.0F),
              "Stop did not discard simulated transforms");
        Check(world.GetComponent<scene::ScriptComponent>(entity)->instanceId == 0,
              "Stop leaked script instance state into edit mode");
    }
}

} // namespace

int main() {
    try {
        TemporaryDirectory temporary;
        TestPlaySession();
        TestInputReplay(temporary.path);
        TestScriptReplay(temporary.path);
        std::cout << "Play session, input replay, and Lua integration checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
