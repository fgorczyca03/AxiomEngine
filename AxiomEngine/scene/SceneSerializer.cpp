#include "AxiomEngine/scene/SceneSerializer.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/scene/Components.h"

#include <fstream>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace axiom::scene {

namespace {

struct SerializedEntity {
    TransformComponent transform{};
    SceneNodeComponent node{};
    rendering::MeshComponent mesh{};
    physics::RigidBodyComponent rigidBody{};
    NameComponent name{};
    ScriptComponent script{};
    bool hasScript{false};
    bool hasName{false};
};

} // namespace

SceneSerializer::SceneSerializer(const assets::AssetRegistry* assetRegistry) : assetRegistry_(assetRegistry) {}

void SceneSerializer::SetAssetRegistry(const assets::AssetRegistry* assetRegistry) { assetRegistry_ = assetRegistry; }

const std::vector<SceneSerializer::ValidationIssue>& SceneSerializer::ValidationIssues() const { return validationIssues_; }

bool SceneSerializer::Save(const ecs::ECSWorld& world, const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "AXIOM_SCENE_V1\n";

    world.ForEach<TransformComponent, SceneNodeComponent, rendering::MeshComponent, physics::RigidBodyComponent>(
        [&](ecs::Entity entity,
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
                << rigidBody.halfExtent.x << " " << rigidBody.halfExtent.y << " " << rigidBody.halfExtent.z << " "
                << static_cast<std::uint32_t>(mesh.primitive) << " ";
            if (const auto* script = world.GetComponent<ScriptComponent>(entity);
                script != nullptr && !script->scriptPath.empty()) {
                out << "1 " << std::quoted(script->scriptPath) << " " << script->enabled;
            } else {
                out << "0";
            }
            if (const auto* name = world.GetComponent<NameComponent>(entity); name != nullptr && !name->value.empty()) {
                out << " 1 " << std::quoted(name->value);
            } else {
                out << " 0";
            }
            out << "\n";
        });

    return out.good();
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

    validationIssues_.clear();

    std::vector<SerializedEntity> entities{};
    std::string line;
    std::size_t lineNumber = 1;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;
        if (tag != "ENTITY") {
            RecordValidationIssue(entities.size(), lineNumber, std::string("Unknown scene record tag: ") + tag);
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

        if (stream.fail()) {
            RecordValidationIssue(entities.size(), lineNumber, "Malformed ENTITY record");
            continue;
        }

        stream >> std::ws;
        if (!stream.eof()) {
            std::string serializedPrimitive;
            if (!(stream >> serializedPrimitive)) {
                RecordValidationIssue(entities.size(), lineNumber, "Malformed primitive shape value");
                continue;
            }
            std::uint32_t primitiveValue = 0;
            const auto [end, error] = std::from_chars(serializedPrimitive.data(),
                                                       serializedPrimitive.data() + serializedPrimitive.size(),
                                                       primitiveValue);
            if (error != std::errc{} || end != serializedPrimitive.data() + serializedPrimitive.size() ||
                !rendering::IsValidPrimitiveShape(primitiveValue)) {
                RecordValidationIssue(entities.size(), lineNumber, "Invalid primitive shape; defaulting to Cube");
                data.mesh.primitive = rendering::PrimitiveShape::Cube;
            } else {
                data.mesh.primitive = static_cast<rendering::PrimitiveShape>(primitiveValue);
            }

            stream >> std::ws;
            if (!stream.eof()) {
                unsigned int hasScript = 0;
                if (!(stream >> hasScript) || hasScript > 1U) {
                    RecordValidationIssue(entities.size(), lineNumber, "Malformed script attachment flag");
                    continue;
                }
                if (hasScript != 0U) {
                    if (!(stream >> std::quoted(data.script.scriptPath) >> data.script.enabled) ||
                        data.script.scriptPath.empty()) {
                        RecordValidationIssue(entities.size(), lineNumber, "Malformed script attachment");
                        continue;
                    }
                    data.hasScript = true;
                }
                stream >> std::ws;
                if (!stream.eof()) {
                    unsigned int hasName = 0;
                    if (!(stream >> hasName) || hasName > 1U) {
                        RecordValidationIssue(entities.size(), lineNumber, "Malformed entity name flag");
                        continue;
                    }
                    if (hasName != 0U) {
                        if (!(stream >> std::quoted(data.name.value)) || data.name.value.empty()) {
                            RecordValidationIssue(entities.size(), lineNumber, "Malformed entity name");
                            continue;
                        }
                        data.hasName = true;
                    }
                    stream >> std::ws;
                    if (!stream.eof()) {
                        RecordValidationIssue(entities.size(), lineNumber, "Unexpected data after ENTITY record");
                        continue;
                    }
                }
            }
        }

        if (!data.hasName) {
            data.name.value = std::string(rendering::PrimitiveShapeName(data.mesh.primitive)) + " " +
                              std::to_string(entities.size() + 1U);
            data.hasName = true;
        }
        data.transform.dirty = true;
        (void)ValidateMeshComponent(data.mesh, entities.size(), lineNumber);
        entities.push_back(data);
    }

    // A valid header with no usable entity records is not a successfully
    // loaded scene. Let the application choose its fallback scene.
    if (entities.empty()) {
        return false;
    }

    for (const SerializedEntity& data : entities) {
        const ecs::Entity entity = world.CreateEntity();
        world.AddComponent(entity, data.transform);
        world.AddComponent(entity, data.node);
        world.AddComponent(entity, data.mesh);
        world.AddComponent(entity, data.rigidBody);
        world.AddComponent(entity, data.name);
        if (data.hasScript) {
            world.AddComponent(entity, data.script);
        }
    }

    return true;
}

bool SceneSerializer::ValidateMeshComponent(rendering::MeshComponent& mesh, std::size_t entityIndex, std::size_t lineNumber) const {
    if (assetRegistry_ == nullptr) {
        return true;
    }

    bool valid = true;
    if (mesh.meshHandle != 0U) {
        const auto meshAsset = assetRegistry_->FindByHandle(static_cast<assets::AssetHandle>(mesh.meshHandle));
        if (!meshAsset.has_value() || meshAsset->type != assets::AssetType::Mesh) {
            RecordValidationIssue(entityIndex, lineNumber, "Invalid mesh asset handle");
            mesh.meshHandle = 0U;
            valid = false;
        }
    }

    if (mesh.materialHandle != 0U) {
        const auto materialAsset = assetRegistry_->FindByHandle(static_cast<assets::AssetHandle>(mesh.materialHandle));
        if (!materialAsset.has_value() || materialAsset->type != assets::AssetType::Material) {
            RecordValidationIssue(entityIndex, lineNumber, "Invalid material asset handle");
            mesh.materialHandle = 0U;
            valid = false;
        }
    }

    return valid;
}

void SceneSerializer::RecordValidationIssue(std::size_t entityIndex, std::size_t lineNumber, std::string message) const {
    validationIssues_.push_back({lineNumber, entityIndex, std::move(message)});
}

} // namespace axiom::scene
