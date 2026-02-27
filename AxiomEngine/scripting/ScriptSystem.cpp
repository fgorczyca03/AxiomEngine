#include "AxiomEngine/scripting/ScriptSystem.h"

#include "AxiomEngine/scene/Components.h"

#include <glm/gtx/quaternion.hpp>

namespace axiom::scripting {

ScriptSystem::ScriptSystem() {
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
}

void ScriptSystem::LoadScript(const std::string& path) {
    lua_.script_file(path);
}

void ScriptSystem::Update(ecs::ECSWorld& world, float dt) {
    const sol::protected_function update = lua_["Update"];
    if (!update.valid()) {
        return;
    }

    world.ForEach<scene::TransformComponent>([&](ecs::Entity entity, scene::TransformComponent& transform) {
        const sol::protected_function_result result = update(entity, dt);
        if (!result.valid()) {
            return;
        }

        const auto deltaYaw = result.get_or(0.0F);
        if (deltaYaw != 0.0F) {
            transform.local.rotation = glm::normalize(glm::angleAxis(deltaYaw, glm::vec3{0.0F, 1.0F, 0.0F}) * transform.local.rotation);
            transform.dirty = true;
        }
    });
}

} // namespace axiom::scripting
