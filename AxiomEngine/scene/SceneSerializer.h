#pragma once

#include "AxiomEngine/assets/AssetRegistry.h"
#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Components.h"

#include <cstddef>
#include <string>
#include <vector>

namespace axiom::scene {

class SceneSerializer {
  public:
    explicit SceneSerializer(const assets::AssetRegistry* assetRegistry = nullptr);

    struct ValidationIssue {
        std::size_t lineNumber{0};
        std::size_t entityIndex{0};
        std::string message{};
    };

    void SetAssetRegistry(const assets::AssetRegistry* assetRegistry);
    bool Save(const ecs::ECSWorld& world, const std::string& path) const;
    bool Load(ecs::ECSWorld& world, const std::string& path) const;
    [[nodiscard]] const std::vector<ValidationIssue>& ValidationIssues() const;

  private:
    [[nodiscard]] bool ValidateMeshComponent(rendering::MeshComponent& mesh, std::size_t entityIndex, std::size_t lineNumber) const;
    void RecordValidationIssue(std::size_t entityIndex, std::size_t lineNumber, std::string message) const;

    const assets::AssetRegistry* assetRegistry_{nullptr};
    mutable std::vector<ValidationIssue> validationIssues_{};
};

} // namespace axiom::scene
