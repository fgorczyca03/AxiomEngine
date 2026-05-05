#include "AxiomEngine/scene/Prefab.h"

#include <fstream>
#include <sstream>

namespace axiom::scene {

namespace {
void WriteTransform(std::ostream& out, const TransformComponent& component, const std::string& tag) {
    const auto& t = component.local;
    out << tag << " " << t.translation.x << " " << t.translation.y << " " << t.translation.z << " " << t.rotation.w << " "
        << t.rotation.x << " " << t.rotation.y << " " << t.rotation.z << " " << t.scale.x << " " << t.scale.y << " " << t.scale.z
        << "\n";
}
}

bool Prefab::Save(const PrefabData& data, const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "AXIOM_PREFAB_V2\n";
    if (data.transform.has_value()) {
        WriteTransform(out, *data.transform, "TRANSFORM");
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
    for (const auto& child : data.children) {
        out << "CHILD " << child.prefabPath << "\n";
        if (child.transformOverride.has_value()) {
            WriteTransform(out, *child.transformOverride, "CHILD_TRANSFORM");
        }
        if (child.nodeOverride.has_value()) {
            out << "CHILD_NODE " << child.nodeOverride->parent << "\n";
        }
        if (child.meshOverride.has_value()) {
            out << "CHILD_MESH " << child.meshOverride->meshHandle << " " << child.meshOverride->materialHandle << "\n";
        }
        if (child.rigidBodyOverride.has_value()) {
            const auto& rb = *child.rigidBodyOverride;
            out << "CHILD_RIGIDBODY " << rb.mass << " " << rb.velocity.x << " " << rb.velocity.y << " " << rb.velocity.z << " " << rb.force.x
                << " " << rb.force.y << " " << rb.force.z << " " << rb.halfExtent.x << " " << rb.halfExtent.y << " " << rb.halfExtent.z
                << "\n";
        }
        out << "CHILD_END\n";
    }

    return true;
}

std::optional<PrefabData> Prefab::Load(const std::string& path) const {
    std::ifstream in(path);
    if (!in.is_open()) {
        return std::nullopt;
    }

    std::string header;
    if (!std::getline(in, header) || (header != "AXIOM_PREFAB_V1" && header != "AXIOM_PREFAB_V2")) {
        return std::nullopt;
    }

    PrefabData data{};
    std::string line;
    PrefabData::ChildInstance* activeChild = nullptr;
    while (std::getline(in, line)) {
        std::istringstream stream(line);
        std::string tag;
        stream >> tag;

        if (tag == "CHILD") {
            std::string path;
            stream >> path;
            data.children.push_back(PrefabData::ChildInstance{});
            data.children.back().prefabPath = path;
            activeChild = &data.children.back();
        } else if (tag == "CHILD_END") {
            activeChild = nullptr;
        } else if (tag == "TRANSFORM" || tag == "CHILD_TRANSFORM") {
            TransformComponent component{};
            stream >> component.local.translation.x >> component.local.translation.y >> component.local.translation.z;
            stream >> component.local.rotation.w >> component.local.rotation.x >> component.local.rotation.y >> component.local.rotation.z;
            stream >> component.local.scale.x >> component.local.scale.y >> component.local.scale.z;
            component.dirty = true;
            if (tag == "CHILD_TRANSFORM" && activeChild != nullptr) {
                activeChild->transformOverride = component;
            } else {
                data.transform = component;
            }
        } else if (tag == "NODE" || tag == "CHILD_NODE") {
            SceneNodeComponent component{};
            stream >> component.parent;
            if (tag == "CHILD_NODE" && activeChild != nullptr) {
                activeChild->nodeOverride = component;
            } else {
                data.node = component;
            }
        } else if (tag == "MESH" || tag == "CHILD_MESH") {
            rendering::MeshComponent component{};
            stream >> component.meshHandle >> component.materialHandle;
            if (tag == "CHILD_MESH" && activeChild != nullptr) {
                activeChild->meshOverride = component;
            } else {
                data.mesh = component;
            }
        } else if (tag == "RIGIDBODY" || tag == "CHILD_RIGIDBODY") {
            physics::RigidBodyComponent component{};
            stream >> component.mass;
            stream >> component.velocity.x >> component.velocity.y >> component.velocity.z;
            stream >> component.force.x >> component.force.y >> component.force.z;
            stream >> component.halfExtent.x >> component.halfExtent.y >> component.halfExtent.z;
            if (tag == "CHILD_RIGIDBODY" && activeChild != nullptr) {
                activeChild->rigidBodyOverride = component;
            } else {
                data.rigidBody = component;
            }
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

    for (const auto& child : data.children) {
        const auto childData = Load(child.prefabPath);
        if (!childData.has_value()) {
            continue;
        }

        PrefabData resolved = *childData;
        if (child.transformOverride.has_value()) {
            resolved.transform = child.transformOverride;
        }
        if (child.nodeOverride.has_value()) {
            resolved.node = child.nodeOverride;
        }
        if (child.meshOverride.has_value()) {
            resolved.mesh = child.meshOverride;
        }
        if (child.rigidBodyOverride.has_value()) {
            resolved.rigidBody = child.rigidBodyOverride;
        }

        std::optional<ecs::Entity> childEntity = Instantiate(world, resolved);
        if (!childEntity.has_value()) {
            continue;
        }

        SceneNodeComponent node{};
        if (SceneNodeComponent* existingNode = world.GetComponent<SceneNodeComponent>(*childEntity); existingNode != nullptr) {
            node = *existingNode;
            node.parent = entity;
            *existingNode = node;
        } else {
            node.parent = entity;
            world.AddComponent(*childEntity, node);
        }
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
