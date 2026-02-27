#include "AxiomEngine/core/Application.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::core {

Application::Application() : sceneGraph_(world_) {
    world_.RegisterComponent<scene::TransformComponent>();
    world_.RegisterComponent<scene::SceneNodeComponent>();
    world_.RegisterComponent<rendering::MeshComponent>();
    world_.RegisterComponent<rendering::MaterialComponent>();
    world_.RegisterComponent<physics::RigidBodyComponent>();
}

void Application::InitializeScene() {
    const auto root = world_.CreateEntity();
    world_.EmplaceComponent<scene::TransformComponent>(root);
    world_.EmplaceComponent<scene::SceneNodeComponent>(root);

    cubeEntity_ = world_.CreateEntity();
    world_.EmplaceComponent<scene::TransformComponent>(cubeEntity_);
    world_.EmplaceComponent<scene::SceneNodeComponent>(cubeEntity_);
    world_.EmplaceComponent<rendering::MeshComponent>(cubeEntity_, 1U, 1U);
    world_.EmplaceComponent<rendering::MaterialComponent>(cubeEntity_);
    world_.EmplaceComponent<physics::RigidBodyComponent>(cubeEntity_);

    sceneGraph_.Attach(root, cubeEntity_);
    scripting_.LoadScript(std::string(AXIOM_ASSET_ROOT) + "/scripts/rotate.lua");
}

int Application::Run() {
    if (!renderer_.Initialize(1280, 720, "Axiom Engine")) {
        return -1;
    }

    InitializeScene();

    constexpr float fixedStep = 1.0F / 120.0F;
    float accumulator = 0.0F;

    while (!renderer_.ShouldClose()) {
        renderer_.PollEvents();
        input_.Update();
        time_.Tick();
        accumulator += static_cast<float>(time_.DeltaSeconds());

        while (accumulator >= fixedStep) {
            physics_.Step(world_, fixedStep);
            scripting_.Update(world_, fixedStep);
            sceneGraph_.UpdateTransforms();
            accumulator -= fixedStep;
        }

        renderer_.BeginFrame();
        renderer_.RenderWorld(world_);
        renderer_.EndFrame();
    }

    renderer_.Shutdown();
    return 0;
}

} // namespace axiom::core
