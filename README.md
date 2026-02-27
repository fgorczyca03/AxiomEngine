# Axiom Engine

Axiom Engine is a modular C++20 engine foundation designed for long-horizon development, not a tutorial sample. The codebase is structured around strict subsystem boundaries and explicit data flow to support future job-based multithreading.

## Architectural Decisions

1. **No hidden global state**: `Application` owns all subsystems and injects references explicitly.
2. **Data-oriented ECS**: entities are integer IDs, components are archetype-local SoA columns, and systems iterate cache-friendly component views.
3. **Deterministic simulation**: fixed-step simulation decoupled from render cadence.
4. **Scene graph + ECS integration**: hierarchy metadata remains in components; transform propagation is explicit and dirty-flag driven.
5. **Renderer isolation**: OpenGL resources are owned via RAII wrappers and consumed through ECS render components.
6. **Future parallelization**: systems declare component access through `ForEach<T...>` signatures, allowing eventual read/write dependency graph scheduling.

## Folder Structure

- `AxiomEngine/core` — app lifecycle, loop policy, time.
- `AxiomEngine/ecs` — archetype ECS storage and iteration.
- `AxiomEngine/scene` — hierarchical transform graph on top of ECS.
- `AxiomEngine/rendering` — OpenGL bootstrap, shader/mesh/camera abstractions, forward pass.
- `AxiomEngine/physics` — rigid bodies, AABB overlap handling, transform sync.
- `AxiomEngine/assets` — shader / glTF / texture loading and cache policy.
- `AxiomEngine/scripting` — Lua VM boundary and update hooks.
- `AxiomEngine/input` — input abstraction layer.
- `Sandbox` — executable host app and runtime assets.

## Build

```bash
cmake -S . -B build
cmake --build build -j
./build/AxiomSandbox
```

## Multithreading-Readiness

- Archetypes are isolated chunk-like storage units.
- `ForEach` establishes exact read/write component sets per system.
- Scene, physics, scripting, and rendering are ordered in an explicit pipeline.
- No singleton subsystems means scheduler/job graph migration can be performed incrementally.
