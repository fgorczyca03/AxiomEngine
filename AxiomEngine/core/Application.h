#pragma once

#include "AxiomEngine/core/JobSystem.h"
#include "AxiomEngine/core/Time.h"
#include "AxiomEngine/assets/AssetRegistry.h"
#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/editor/EditorLayer.h"
#include "AxiomEngine/input/InputSystem.h"
#include "AxiomEngine/physics/PhysicsSystem.h"
#include "AxiomEngine/rendering/Renderer.h"
#include "AxiomEngine/scene/Prefab.h"
#include "AxiomEngine/scene/SceneGraph.h"
#include "AxiomEngine/scene/SceneSerializer.h"
#include "AxiomEngine/scripting/ScriptSystem.h"

namespace axiom::core {

class Application {
  public:
    Application();
    int Run();

  private:
    void InitializeScene();

    Time time_{};
    ecs::ECSWorld world_{};
    rendering::Renderer renderer_{};
    physics::PhysicsSystem physics_{};
    input::InputSystem input_{};
    scripting::ScriptSystem scripting_{};
    scene::SceneGraph sceneGraph_;
    assets::AssetRegistry assetRegistry_{};
    scene::SceneSerializer sceneSerializer_{&assetRegistry_};
    scene::Prefab prefab_{};
    ThreadPoolJobSystem jobs_{};
    editor::EditorLayer editor_{};
    ecs::Entity cubeEntity_{0};
};

} // namespace axiom::core
