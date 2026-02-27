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
        auto transformView = lua_.create_table();
        transformView["x"] = transform.local.translation.x;
        transformView["y"] = transform.local.translation.y;
        transformView["z"] = transform.local.translation.z;

        const sol::protected_function_result result = update(entity, dt, transformView);
        if (!result.valid()) {
            return;
        }

        transform.local.translation.x = transformView.get_or("x", transform.local.translation.x);
        transform.local.translation.y = transformView.get_or("y", transform.local.translation.y);
        transform.local.translation.z = transformView.get_or("z", transform.local.translation.z);

        if (result.get_type() == sol::type::number) {
            const float deltaYaw = result.get<float>();
            if (deltaYaw != 0.0F) {
                transform.local.rotation = glm::normalize(glm::angleAxis(deltaYaw, glm::vec3{0.0F, 1.0F, 0.0F}) * transform.local.rotation);
            }
            transform.dirty = true;
            return;
        }

        if (result.get_type() == sol::type::table) {
            const sol::table out = result;
            transform.local.translation.x = out.get_or("x", transform.local.translation.x);
            transform.local.translation.y = out.get_or("y", transform.local.translation.y);
            transform.local.translation.z = out.get_or("z", transform.local.translation.z);
            transform.dirty = true;
        }
    });
}

} // namespace axiom::scripting
