#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <optional>
#include <string>

namespace axiom::scene {

struct PrefabData {
    std::optional<TransformComponent> transform{};
    std::optional<SceneNodeComponent> node{};
    std::optional<rendering::MeshComponent> mesh{};
    std::optional<physics::RigidBodyComponent> rigidBody{};
};

class Prefab {
  public:
    bool Save(const PrefabData& data, const std::string& path) const;
    std::optional<PrefabData> Load(const std::string& path) const;

    std::optional<ecs::Entity> Instantiate(ecs::ECSWorld& world, const PrefabData& data) const;
    std::optional<ecs::Entity> InstantiateFromFile(ecs::ECSWorld& world, const std::string& path) const;
};

} // namespace axiom::scene
