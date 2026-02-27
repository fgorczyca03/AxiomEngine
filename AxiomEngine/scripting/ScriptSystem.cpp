#include "AxiomEngine/scripting/ScriptSystem.h"

#include "AxiomEngine/scene/Components.h"

namespace axiom::scripting {

ScriptSystem::ScriptSystem() {
    lua_.open_libraries(sol::lib::base, sol::lib::math);
}

void ScriptSystem::LoadScript(const std::string& path) {
    lua_.script_file(path);
}

void ScriptSystem::Update(ecs::ECSWorld& world, float dt) {
    auto update = lua_["Update"];
    if (!update.valid()) {
        return;
    }

    world.ForEach<scene::TransformComponent>([&](ecs::Entity entity, scene::TransformComponent& transform) {
        auto table = lua_.create_table();
        table["x"] = transform.local.translation.x;
        table["y"] = transform.local.translation.y;
        table["z"] = transform.local.translation.z;

        const sol::table result = update(entity, dt, table);
        transform.local.translation = {result["x"].get_or(transform.local.translation.x), result["y"].get_or(transform.local.translation.y), result["z"].get_or(transform.local.translation.z)};
        transform.dirty = true;
    });
}

} // namespace axiom::scripting
