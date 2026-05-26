#pragma once

#include "AxiomEngine/assets/AssetRegistry.h"
#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Components.h"

#include <string>

namespace axiom::scene {

class SceneSerializer {
  public:
    explicit SceneSerializer(const assets::AssetRegistry* assetRegistry = nullptr);

    void SetAssetRegistry(const assets::AssetRegistry* assetRegistry);
    bool Save(const ecs::ECSWorld& world, const std::string& path) const;
    bool Load(ecs::ECSWorld& world, const std::string& path) const;

  private:
    [[nodiscard]] bool ValidateMeshComponent(rendering::MeshComponent& mesh) const;

    const assets::AssetRegistry* assetRegistry_{nullptr};
};

} // namespace axiom::scene
