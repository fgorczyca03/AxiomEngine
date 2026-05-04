#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"

#include <string>

namespace axiom::scene {

class SceneSerializer {
  public:
    bool Save(const ecs::ECSWorld& world, const std::string& path) const;
    bool Load(ecs::ECSWorld& world, const std::string& path) const;
};

} // namespace axiom::scene
