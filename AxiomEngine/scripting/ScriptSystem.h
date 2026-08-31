#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

#include <sol/sol.hpp>

#include <string>
#include <vector>

namespace axiom::scripting {

class ScriptSystem {
  public:
    struct ScriptError {
        std::string phase{};
        std::string message{};
    };

    ScriptSystem();
    [[nodiscard]] bool LoadScript(const std::string& path);
    void Update(ecs::ECSWorld& world, float dt);

    [[nodiscard]] bool HasErrors() const;
    [[nodiscard]] const std::vector<ScriptError>& Errors() const;
    void ClearErrors();

  private:
    void RecordError(std::string phase, std::string message);

    sol::state lua_{};
    std::vector<ScriptError> errors_{};
};

} // namespace axiom::scripting
