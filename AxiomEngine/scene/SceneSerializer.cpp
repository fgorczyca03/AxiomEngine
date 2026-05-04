#include "AxiomEngine/scene/SceneSerializer.h"

#include <fstream>

namespace axiom::scene {

bool SceneSerializer::Save(const ecs::ECSWorld&, const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "# AxiomScene v1\n";
    out << "# Scene serialization scaffold.\n";
    return true;
}

bool SceneSerializer::Load(ecs::ECSWorld&, const std::string& path) const {
    std::ifstream in(path);
    return in.is_open();
}

} // namespace axiom::scene
