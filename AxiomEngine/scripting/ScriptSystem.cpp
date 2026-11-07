#include "AxiomEngine/scripting/ScriptSystem.h"

#include "AxiomEngine/scene/Components.h"

#include <glm/gtx/quaternion.hpp>

#include <cmath>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace axiom::scripting {

ScriptSystem::ScriptSystem() {
    lua_.open_libraries(sol::lib::base, sol::lib::math);
}

void ScriptSystem::SetAssetRoot(std::filesystem::path assetRoot) { assetRoot_ = std::move(assetRoot); }

std::filesystem::path ScriptSystem::ResolveScriptPath(const std::string& scriptPath) const {
    const std::filesystem::path relativePath(scriptPath);
    // Scripts must stay inside Sandbox/scripts; absolute paths and parent traversal are rejected.
    if (relativePath.empty() || relativePath.is_absolute() || relativePath.has_root_name()) {
        return {};
    }

    for (const auto& part : relativePath) {
        if (part == "..") {
            return {};
        }
    }

    const std::filesystem::path normalized = relativePath.lexically_normal();
    if (normalized.begin() == normalized.end() || *normalized.begin() != "scripts" || normalized.extension() != ".lua") {
        return {};
    }
    return assetRoot_ / normalized;
}

bool ScriptSystem::LoadScript(ecs::Entity entity, const std::string& scriptPath) {
    const std::filesystem::path absolutePath = ResolveScriptPath(scriptPath);
    if (absolutePath.empty()) {
        RecordError("load", "Entity " + std::to_string(entity) + " has an invalid script path: " + scriptPath);
        return false;
    }

    std::error_code errorCode;
    const auto writeTime = std::filesystem::last_write_time(absolutePath, errorCode);
    if (errorCode) {
        RecordError("load", "Could not read script for Entity " + std::to_string(entity) + ": " + scriptPath);
        return false;
    }

    // Inherit Lua's intentionally small base/math global set, but give this object private script variables.
    sol::environment environment(lua_, sol::create, lua_.globals());
    const sol::protected_function_result result = lua_.safe_script_file(absolutePath.string(), environment);
    if (!result.valid()) {
        const sol::error scriptError = result;
        RecordError("load", "Entity " + std::to_string(entity) + " (" + scriptPath + "): " + scriptError.what());
        return false;
    }

    sol::object updateObject = environment.get<sol::object>("Update");
    if (!updateObject.is<sol::function>()) {
        RecordError("load", "Entity " + std::to_string(entity) + " script must define Update(entity, dt, transform): " +
                                 scriptPath);
        return false;
    }

    ScriptInstance instance{};
    instance.scriptPath = scriptPath;
    instance.lastWriteTime = writeTime;
    instance.environment = std::move(environment);
    instance.update = instance.environment.get<sol::protected_function>("Update");
    instances_.insert_or_assign(entity, std::move(instance));
    return true;
}

void ScriptSystem::Update(ecs::ECSWorld& world, float dt) {
    std::unordered_set<ecs::Entity> activeInstances{};
    world.ForEach<scene::TransformComponent, scene::ScriptComponent>(
        [&](ecs::Entity entity, scene::TransformComponent& transform, scene::ScriptComponent& script) {
            if (!script.enabled || script.scriptPath.empty()) {
                return;
            }

            if (script.instanceId == 0) {
                script.instanceId = nextInstanceId_++;
            }
            activeInstances.insert(entity);
            auto instance = instances_.find(entity);
            if (instance == instances_.end() || instance->second.scriptPath != script.scriptPath ||
                instance->second.instanceId != script.instanceId) {
                if (!LoadScript(entity, script.scriptPath)) {
                    script.enabled = false;
                    script.lastError = errors_.empty() ? "Could not load the script." : errors_.back().message;
                    return;
                }
                instance = instances_.find(entity);
                instance->second.instanceId = script.instanceId;
                script.lastError.clear();
            } else {
                std::error_code errorCode;
                // Poll source time for hot reload without rereading and compiling the file on every update.
                const auto writeTime = std::filesystem::last_write_time(ResolveScriptPath(script.scriptPath), errorCode);
                if (errorCode) {
                    RecordError("load", "Could not read script for Entity " + std::to_string(entity) + ": " +
                                             script.scriptPath);
                    script.enabled = false;
                    script.lastError = errors_.back().message;
                    return;
                }
                if (writeTime != instance->second.lastWriteTime) {
                    if (!LoadScript(entity, script.scriptPath)) {
                        script.enabled = false;
                        script.lastError = errors_.empty() ? "Could not reload the script." : errors_.back().message;
                        return;
                    }
                    instance = instances_.find(entity);
                    instance->second.instanceId = script.instanceId;
                    script.lastError.clear();
                }
            }

            // Lua edits this temporary table; C++ copies validated values back after Update returns.
            auto table = lua_.create_table();
            table["x"] = transform.local.translation.x;
            table["y"] = transform.local.translation.y;
            table["z"] = transform.local.translation.z;
            table["rotationW"] = transform.local.rotation.w;
            table["rotationX"] = transform.local.rotation.x;
            table["rotationY"] = transform.local.rotation.y;
            table["rotationZ"] = transform.local.rotation.z;
            table["scaleX"] = transform.local.scale.x;
            table["scaleY"] = transform.local.scale.y;
            table["scaleZ"] = transform.local.scale.z;

            const sol::protected_function_result result = instance->second.update(entity, dt, table);
            if (!result.valid()) {
                const sol::error scriptError = result;
                RecordError("update", "Entity " + std::to_string(entity) + " (" + script.scriptPath + "): " +
                                         scriptError.what());
                script.enabled = false;
                // Stop calling a broken script every tick; the Inspector can re-enable it after fixing the cause.
                script.lastError = errors_.back().message;
                return;
            }

            const auto readNumber = [&table](const char* key, float fallback) {
                const sol::object value = table.get<sol::object>(key);
                return value.get_type() == sol::type::number ? value.as<float>() : fallback;
            };
            transform.local.translation = {
                readNumber("x", transform.local.translation.x),
                readNumber("y", transform.local.translation.y),
                readNumber("z", transform.local.translation.z),
            };
            const glm::quat rotation{
                readNumber("rotationW", transform.local.rotation.w),
                readNumber("rotationX", transform.local.rotation.x),
                readNumber("rotationY", transform.local.rotation.y),
                readNumber("rotationZ", transform.local.rotation.z),
            };
            const float rotationLengthSquared = glm::dot(rotation, rotation);
            if (std::isfinite(rotationLengthSquared) && rotationLengthSquared > 0.000001F) {
                transform.local.rotation = glm::normalize(rotation);
            }
            transform.local.scale = {
                readNumber("scaleX", transform.local.scale.x),
                readNumber("scaleY", transform.local.scale.y),
                readNumber("scaleZ", transform.local.scale.z),
            };
            transform.dirty = true;
            script.lastError.clear();
    });

    // Drop Lua environments after a script is detached, disabled, or its entity is destroyed.
    for (auto it = instances_.begin(); it != instances_.end();) {
        if (!activeInstances.contains(it->first)) {
            it = instances_.erase(it);
        } else {
            ++it;
        }
    }
}

bool ScriptSystem::HasErrors() const { return !errors_.empty(); }

const std::vector<ScriptSystem::ScriptError>& ScriptSystem::Errors() const { return errors_; }

void ScriptSystem::ClearErrors() { errors_.clear(); }

void ScriptSystem::RecordError(std::string phase, std::string message) {
    errors_.push_back({std::move(phase), std::move(message)});
}

} // namespace axiom::scripting
