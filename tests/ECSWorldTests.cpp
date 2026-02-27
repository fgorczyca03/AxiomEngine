#include "AxiomEngine/ecs/ECSWorld.h"

#include <cassert>

namespace {
struct Position {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct Velocity {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};
} // namespace

int main() {
    axiom::ecs::ECSWorld world;
    world.RegisterComponent<Position>();
    world.RegisterComponent<Velocity>();

    const auto e0 = world.CreateEntity();
    const auto e1 = world.CreateEntity();

    world.EmplaceComponent<Position>(e0, Position{1.0F, 2.0F, 3.0F});
    world.EmplaceComponent<Velocity>(e0, Velocity{0.1F, 0.2F, 0.3F});
    world.EmplaceComponent<Position>(e1, Position{10.0F, 20.0F, 30.0F});

    int moved = 0;
    world.ForEach<Position, Velocity>([&](axiom::ecs::Entity, Position& p, const Velocity& v) {
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
        ++moved;
    });

    assert(moved == 1);

    const auto* pos0 = world.GetComponent<Position>(e0);
    assert(pos0 != nullptr);
    assert(pos0->x > 1.09F && pos0->x < 1.11F);

    world.RemoveComponent<Velocity>(e0);
    const auto* vel0 = world.GetComponent<Velocity>(e0);
    assert(vel0 == nullptr);

    world.DestroyEntity(e1);
    const auto* pos1 = world.GetComponent<Position>(e1);
    assert(pos1 == nullptr);

    return 0;
}
