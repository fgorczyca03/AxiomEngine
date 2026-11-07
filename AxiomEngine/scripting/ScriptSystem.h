#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

#include <sol/sol.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace axiom::scripting {

// Runs one isolated Lua environment per attached entity and reports failures back to the Inspector.
class ScriptSystem {
  public:
    struct ScriptError {
        std::string phase{};
        std::string message{};
    };

    ScriptSystem();
    void SetAssetRoot(std::filesystem::path assetRoot);
    void Update(ecs::ECSWorld& world, float dt);

    [[nodiscard]] bool HasErrors() const;
    [[nodiscard]] const std::vector<ScriptError>& Errors() const;
    void ClearErrors();

  private:
    struct ScriptInstance {
        std::string scriptPath{};
        std::uint64_t instanceId{0};
        std::filesystem::file_time_type lastWriteTime{};
        float timeUntilReloadCheck{0.0F};
        bool activeThisUpdate{false};
        // Each entity gets its own globals, even when several entities run the same file.
        sol::environment environment{};
        sol::protected_function update{};
    };

    [[nodiscard]] bool LoadScript(ecs::Entity entity, const std::string& scriptPath);
    [[nodiscard]] std::filesystem::path ResolveScriptPath(const std::string& scriptPath) const;
    void RecordError(std::string phase, std::string message);

    sol::state lua_{};
    std::filesystem::path assetRoot_{};
    std::unordered_map<ecs::Entity, ScriptInstance> instances_{};
    std::uint64_t nextInstanceId_{1};
    std::vector<ScriptError> errors_{};
};

} // namespace axiom::scripting
