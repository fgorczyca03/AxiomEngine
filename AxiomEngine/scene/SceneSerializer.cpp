#include "AxiomEngine/scene/SceneSerializer.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <fstream>
#include <sstream>
#include <vector>

namespace axiom::scene {

namespace {

struct SerializedEntity {
    TransformComponent transform{};
    SceneNodeComponent node{};
    rendering::MeshComponent mesh{};
    physics::RigidBodyComponent rigidBody{};
};

} // namespace

SceneSerializer::SceneSerializer(const assets::AssetRegistry* assetRegistry) : assetRegistry_(assetRegistry) {}

void SceneSerializer::SetAssetRegistry(const assets::AssetRegistry* assetRegistry) { assetRegistry_ = assetRegistry; }

bool SceneSerializer::Save(const ecs::ECSWorld& world, const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "AXIOM_SCENE_V1\n";

    const_cast<ecs::ECSWorld&>(world).ForEach<TransformComponent, SceneNodeComponent, rendering::MeshComponent, physics::RigidBodyComponent>(
        [&](ecs::Entity,
            const TransformComponent& transform,
            const SceneNodeComponent& node,
            const rendering::MeshComponent& mesh,
            const physics::RigidBodyComponent& rigidBody) {
            out << "ENTITY "
                << transform.local.translation.x << " " << transform.local.translation.y << " " << transform.local.translation.z << " "
                << transform.local.rotation.w << " " << transform.local.rotation.x << " " << transform.local.rotation.y << " " << transform.local.rotation.z << " "
                << transform.local.scale.x << " " << transform.local.scale.y << " " << transform.local.scale.z << " "
                << node.parent << " "
                << mesh.meshHandle << " " << mesh.materialHandle << " "
                << rigidBody.mass << " "
                << rigidBody.velocity.x << " " << rigidBody.velocity.y << " " << rigidBody.velocity.z << " "
                << rigidBody.force.x << " " << rigidBody.force.y << " " << rigidBody.force.z << " "
                << rigidBody.halfExtent.x << " " << rigidBody.halfExtent.y << " " << rigidBody.halfExtent.z
                << "\n";
        });

    return true;
}

bool SceneSerializer::Load(ecs::ECSWorld& world, const std::string& path) const {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string header;
    if (!std::getline(in, header) || header != "AXIOM_SCENE_V1") {
        return false;
    }

    std::vector<SerializedEntity> entities{};
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;
        if (tag != "ENTITY") {
            continue;
        }

        SerializedEntity data{};
        stream >> data.transform.local.translation.x >> data.transform.local.translation.y >> data.transform.local.translation.z;
        stream >> data.transform.local.rotation.w >> data.transform.local.rotation.x >> data.transform.local.rotation.y >> data.transform.local.rotation.z;
        stream >> data.transform.local.scale.x >> data.transform.local.scale.y >> data.transform.local.scale.z;
        stream >> data.node.parent;
        stream >> data.mesh.meshHandle >> data.mesh.materialHandle;
        stream >> data.rigidBody.mass;
        stream >> data.rigidBody.velocity.x >> data.rigidBody.velocity.y >> data.rigidBody.velocity.z;
        stream >> data.rigidBody.force.x >> data.rigidBody.force.y >> data.rigidBody.force.z;
        stream >> data.rigidBody.halfExtent.x >> data.rigidBody.halfExtent.y >> data.rigidBody.halfExtent.z;

        data.transform.dirty = true;
        ValidateMeshComponent(data.mesh);
        entities.push_back(data);
    }

    for (const SerializedEntity& data : entities) {
        const ecs::Entity entity = world.CreateEntity();
        world.AddComponent(entity, data.transform);
        world.AddComponent(entity, data.node);
        world.AddComponent(entity, data.mesh);
        world.AddComponent(entity, data.rigidBody);
    }

    return true;
}

bool SceneSerializer::ValidateMeshComponent(rendering::MeshComponent& mesh) const {
    if (assetRegistry_ == nullptr) {
        return true;
    }

    bool valid = true;
    if (mesh.meshHandle != 0U) {
        const auto meshAsset = assetRegistry_->FindByHandle(static_cast<assets::AssetHandle>(mesh.meshHandle));
        if (!meshAsset.has_value() || meshAsset->type != assets::AssetType::Mesh) {
            mesh.meshHandle = 0U;
            valid = false;
        }
    }

    if (mesh.materialHandle != 0U) {
        const auto materialAsset = assetRegistry_->FindByHandle(static_cast<assets::AssetHandle>(mesh.materialHandle));
        if (!materialAsset.has_value() || materialAsset->type != assets::AssetType::Material) {
            mesh.materialHandle = 0U;
            valid = false;
        }
    }

    return valid;
}

} // namespace axiom::scene
