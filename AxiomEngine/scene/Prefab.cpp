#include "AxiomEngine/scene/Prefab.h"

#include <fstream>
#include <sstream>

namespace axiom::scene {

bool Prefab::Save(const PrefabData& data, const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "AXIOM_PREFAB_V1\n";
    if (data.transform.has_value()) {
        const auto& t = data.transform->local;
        out << "TRANSFORM "
            << t.translation.x << " " << t.translation.y << " " << t.translation.z << " "
            << t.rotation.w << " " << t.rotation.x << " " << t.rotation.y << " " << t.rotation.z << " "
            << t.scale.x << " " << t.scale.y << " " << t.scale.z << "\n";
    }
    if (data.node.has_value()) {
        out << "NODE " << data.node->parent << "\n";
    }
    if (data.mesh.has_value()) {
        out << "MESH " << data.mesh->meshHandle << " " << data.mesh->materialHandle << "\n";
    }
    if (data.rigidBody.has_value()) {
        const auto& rb = *data.rigidBody;
        out << "RIGIDBODY "
            << rb.mass << " "
            << rb.velocity.x << " " << rb.velocity.y << " " << rb.velocity.z << " "
            << rb.force.x << " " << rb.force.y << " " << rb.force.z << " "
            << rb.halfExtent.x << " " << rb.halfExtent.y << " " << rb.halfExtent.z << "\n";
    }

    return true;
}

std::optional<PrefabData> Prefab::Load(const std::string& path) const {
    std::ifstream in(path);
    if (!in.is_open()) {
        return std::nullopt;
    }

    std::string header;
    if (!std::getline(in, header) || header != "AXIOM_PREFAB_V1") {
        return std::nullopt;
    }

    PrefabData data{};
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream stream(line);
        std::string tag;
        stream >> tag;

        if (tag == "TRANSFORM") {
            TransformComponent component{};
            stream >> component.local.translation.x >> component.local.translation.y >> component.local.translation.z;
            stream >> component.local.rotation.w >> component.local.rotation.x >> component.local.rotation.y >> component.local.rotation.z;
            stream >> component.local.scale.x >> component.local.scale.y >> component.local.scale.z;
            component.dirty = true;
            data.transform = component;
        } else if (tag == "NODE") {
            SceneNodeComponent component{};
            stream >> component.parent;
            data.node = component;
        } else if (tag == "MESH") {
            rendering::MeshComponent component{};
            stream >> component.meshHandle >> component.materialHandle;
            data.mesh = component;
        } else if (tag == "RIGIDBODY") {
            physics::RigidBodyComponent component{};
            stream >> component.mass;
            stream >> component.velocity.x >> component.velocity.y >> component.velocity.z;
            stream >> component.force.x >> component.force.y >> component.force.z;
            stream >> component.halfExtent.x >> component.halfExtent.y >> component.halfExtent.z;
            data.rigidBody = component;
        }
    }

    return data;
}

std::optional<ecs::Entity> Prefab::Instantiate(ecs::ECSWorld& world, const PrefabData& data) const {
    const ecs::Entity entity = world.CreateEntity();

    if (data.transform.has_value()) {
        world.AddComponent(entity, *data.transform);
    }
    if (data.node.has_value()) {
        world.AddComponent(entity, *data.node);
    }
    if (data.mesh.has_value()) {
        world.AddComponent(entity, *data.mesh);
    }
    if (data.rigidBody.has_value()) {
        world.AddComponent(entity, *data.rigidBody);
    }

    return entity;
}

std::optional<ecs::Entity> Prefab::InstantiateFromFile(ecs::ECSWorld& world, const std::string& path) const {
    const auto data = Load(path);
    if (!data.has_value()) {
        return std::nullopt;
    }
    return Instantiate(world, *data);
}

} // namespace axiom::scene
