# Axiom Engine

A modular C++20 engine core with explicit module boundaries for long-term systems research.

## High-Level Architecture

Axiom is split into simulation and presentation layers that communicate through ECS data, with no hidden global state:

- **Core runtime** drives fixed-step simulation and variable-rate rendering.
- **ECS layer** owns entities/components in archetype-packed storage.
- **Scene layer** resolves transform hierarchies and world matrices.
- **Systems layer** (physics/scripting/input) mutates ECS components.
- **Render layer** consumes read-only frame snapshots through a frame-graph pass model.
- **Tooling layer** (editor + profiling) observes frame state without owning gameplay data.

## Folder Structure

- `AxiomEngine/core`: app loop, deterministic timing, lifetime control, job system contract.
- `AxiomEngine/ecs`: archetype ECS with SoA component columns and signature-based archetypes.
- `AxiomEngine/scene`: transform hierarchy, world matrix propagation, scene serialization scaffold.
- `AxiomEngine/rendering`: OpenGL bootstrap, shader, mesh, camera, forward renderer, frame graph scaffold.
- `AxiomEngine/physics`: minimal ECS-driven rigid body stepper.
- `AxiomEngine/assets`: asset caching interfaces and asset registry/import scaffolding.
- `AxiomEngine/scripting`: Lua bridge.
- `AxiomEngine/input`: input polling abstraction with action mapping.
- `AxiomEngine/editor`: ImGui-compatible editor layer scaffold.
- `AxiomEngine/profiling`: pluggable profiler hooks.
- `Sandbox`: sample application, shaders, scripts, and scene/input content.

## Build

From repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release --fresh
cmake --build build --target AxiomRuntime --config Release
```

Run:

```bash
./build/AxiomRuntime
```

## Linux backend configuration

Linux builds support explicit GLFW backend selection via:

- `AXIOM_GLFW_USE_WAYLAND`
- `AXIOM_GLFW_USE_X11`

Examples:

```bash
# Wayland
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release --fresh \
  -DAXIOM_GLFW_USE_WAYLAND=ON -DAXIOM_GLFW_USE_X11=OFF

# X11
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release --fresh \
  -DAXIOM_GLFW_USE_WAYLAND=OFF -DAXIOM_GLFW_USE_X11=ON
```

If you reconfigure often, `--fresh` avoids stale cache variables from prior builds.

## Development Focus

### Near term (next 1-2 milestones)

- Improve material/light/shadow quality under the frame-graph path.
- Expand collision coverage and deterministic Sandbox controls.
- Improve Lua scripting ergonomics and surfaced script errors.
- Deepen scene + asset handle validation across authored content.
- Expand input mappings (mouse/gamepad/rebinding UI).

### Mid term (engine maturity)

- Add worker-thread backend for `IJobSystem` and parallelize key phases.
- Evolve frame-graph resource lifetime/transient planning.
- Build incremental, dependency-aware asset import recipes.
- Deliver core editor hierarchy/inspector/gizmos + play-in-editor loop.
- Add nested prefab support with override workflows.

### Long term (production readiness)

- Add async streaming/world partition and strict memory budgets.
- Keep OpenGL reference backend while adding another backend.
- Introduce deterministic snapshot/rollback-friendly replication foundations.
- Add CI quality gates for formatting/lint/build/tests/content/perf regression.
- Ship end-to-end sample projects for canonical workflows.

## Additional docs

- `docs/CODING_STYLE.rst` for coding conventions used across the project.

## Development Focus

The project is currently prioritizing the following initiatives:
- `AXIOM_GLFW_USE_WAYLAND`
- `AXIOM_GLFW_USE_X11`

### Near term (next 1-2 milestones)
- Raise rendering quality with material parameters, directional/point lights, and a basic shadow-map pass under the existing frame-graph model.
- Improve simulation iteration with broader collision support (AABB/capsule + broadphase) and deterministic Sandbox controls.
- Expand Lua scripting ergonomics beyond `Update(entity, dt, position)` to richer transform/input/spawn/tag APIs with surfaced script errors.
- Stabilize authored content flow by deepening `SceneSerializer` + `AssetRegistry` integration, with asset handle validation at load.
- Raise rendering quality with material parameters, directional/point lights, and a basic shadow-map pass under the existing frame-graph model.
- Improve simulation iteration with broader collision support (AABB/capsule + broadphase) and deterministic Sandbox controls.
- Expand Lua scripting ergonomics beyond `Update(entity, dt, position)` to richer transform/input/spawn/tag APIs with surfaced script errors.
- Add mouse/gamepad action bindings, device sensitivity presets, and runtime rebinding UI for serialized action maps.

### Mid term (engine maturity)
- Implement a worker-thread pool backend for `IJobSystem` and parallelize transform propagation, script updates, and render extraction.
- Evolve frame-graph execution toward explicit resource lifetime tracking and transient allocation planning for deferred/compute paths.
- Build `AssetImporter` recipes, dependency tracking, and incremental reimport for reproducible/cached cooked outputs.
- Deliver core editor hierarchy/inspector/gizmos and a play-in-editor loop on top of `EditorLayer` with profiling overlays.
- Add nested prefab support with override tracking plus diff/apply authoring workflows.

### Long term (production readiness)
- Add asynchronous streaming, world partition/chunk loading, and explicit CPU/GPU memory budget enforcement.
- Preserve OpenGL as a reference backend while adding at least one additional backend (e.g., Vulkan/Metal).
- Introduce deterministic snapshot/rollback-friendly ECS replication foundations.
- Establish CI quality gates for formatting/lint/build/tests, content validation, and performance regression checks.
- Ship end-to-end sample projects that demonstrate canonical engine workflows.
