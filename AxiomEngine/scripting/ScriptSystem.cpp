#include "AxiomEngine/scripting/ScriptSystem.h"

#include "AxiomEngine/scene/Components.h"

#include <utility>

namespace axiom::scripting {

ScriptSystem::ScriptSystem() {
    lua_.open_libraries(sol::lib::base, sol::lib::math);
}

bool ScriptSystem::LoadScript(const std::string& path) {
    sol::load_result script = lua_.load_file(path);
    if (!script.valid()) {
        const sol::error error = script;
        RecordError("load", error.what());
        return false;
    }

    const sol::protected_function_result result = script();
    if (!result.valid()) {
        const sol::error error = result;
        RecordError("execute", error.what());
        return false;
    }

    return true;
}

void ScriptSystem::Update(ecs::ECSWorld& world, float dt) {
    sol::protected_function update = lua_["Update"];
    if (!update.valid()) {
        return;
    }

    world.ForEach<scene::TransformComponent>([&](ecs::Entity entity, scene::TransformComponent& transform) {
        auto table = lua_.create_table();
        table["x"] = transform.local.translation.x;
        table["y"] = transform.local.translation.y;
        table["z"] = transform.local.translation.z;

        const sol::protected_function_result result = update(entity, dt, table);
        if (!result.valid()) {
            const sol::error error = result;
            RecordError("update", error.what());
            return;
        }

        const sol::table resultTable = result.get<sol::table>();
        transform.local.translation = {resultTable["x"].get_or(transform.local.translation.x), resultTable["y"].get_or(transform.local.translation.y), resultTable["z"].get_or(transform.local.translation.z)};
        transform.dirty = true;
    });
}

bool ScriptSystem::HasErrors() const { return !errors_.empty(); }

const std::vector<ScriptSystem::ScriptError>& ScriptSystem::Errors() const { return errors_; }

void ScriptSystem::ClearErrors() { errors_.clear(); }

void ScriptSystem::RecordError(std::string phase, std::string message) {
    errors_.push_back({std::move(phase), std::move(message)});
}

} // namespace axiom::scripting
