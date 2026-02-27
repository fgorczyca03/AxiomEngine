#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

#include <sol/sol.hpp>

#include <string>

namespace axiom::scripting {

class ScriptSystem {
  public:
    ScriptSystem();
    void LoadScript(const std::string& path);
    void Update(ecs::ECSWorld& world, float dt);

  private:
    sol::state lua_{};
};

} // namespace axiom::scripting
