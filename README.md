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

This follows the architectural spirit of id Tech/Frostbite/Source: strict subsystem contracts, data-oriented runtime, and explicit scheduling seams.

## Major Design Decisions

### ECS layout: archetype SoA

The world uses signature-indexed archetypes where each component type is stored in its own contiguous column per archetype. This keeps cache-friendly linear iteration for systems, and enables deterministic migration when signatures change.

### Render pipeline choice: forward renderer + frame-graph scaffold

The renderer remains a minimal forward path today for fast iteration, but now routes execution through a `FrameGraph` abstraction. Passes declare resource reads/writes and execute in graph order. This makes future migration to deferred, async compute, and transient attachment allocation possible without rewriting app-level render flow.

### Physics integration: ECS-first deterministic step

Physics remains ECS-driven with fixed-step updates and direct transform synchronization. This keeps simulation deterministic and avoids hidden ownership splits between game objects and rigid bodies. The new job interface schedules each simulation phase as explicit tasks, preparing for multithreaded execution later.

## Folder Structure

- `AxiomEngine/core`: app loop, deterministic timing, lifetime control, job system contract.
- `AxiomEngine/ecs`: archetype ECS with SoA component columns and signature-based archetypes.
- `AxiomEngine/scene`: transform hierarchy, world matrix propagation, and scene serialization scaffold.
- `AxiomEngine/rendering`: OpenGL bootstrap, shader, mesh, camera, forward renderer, frame graph scaffold.
- `AxiomEngine/physics`: minimal ECS-driven rigid body stepper.
- `AxiomEngine/assets`: asset caching interfaces and asset registry scaffolding.
- `AxiomEngine/scripting`: Lua bridge.
- `AxiomEngine/input`: input polling abstraction.
- `AxiomEngine/editor`: ImGui-compatible editor layer scaffold.
- `AxiomEngine/profiling`: pluggable profiler hooks (Tracy-style scope API).
- `Sandbox`: sample application, shaders, and scripts.

## Architecture Highlights

### ECS

Entities are integral IDs. Components are assigned runtime type IDs and packed into archetype-local columns. Each archetype stores one SoA vector per component type in that archetype. Moving entities across signatures migrates row data between archetypes, preserving tight linear iteration for systems.

### Scene and Transform Propagation

Scene nodes carry parent IDs and transform components. Dirty local transforms recompute world transforms on update. This keeps simulation writes local and render reads consistent.

### Renderer + Frame Graph

OpenGL 4.5 core profile via GLFW + GLAD. The renderer owns device objects (shader/mesh/camera) and executes passes through a lightweight frame graph. The default graph contains a forward opaque pass that samples ECS transform + mesh components.

### Physics

A minimal semi-implicit Euler solver updates rigid body velocity/position and syncs transform components directly, with simple floor collision response.

### Lua scripting

Lua `Update(entity, dt, position)` is called per entity each simulation tick. Script outputs are applied to ECS transform data.

### Job system scaffold

`IJobSystem` defines an engine-level scheduling contract (`Enqueue/Flush/WorkerCount`). `InlineJobSystem` is a deterministic single-thread backend used by the app loop today, preserving behavior while exposing job boundaries for a future worker-pool backend.

### Editor scaffold

`EditorLayer` wraps an ImGui-style lifecycle (`Initialize/BeginFrame/Draw/EndFrame/Shutdown`). It compiles without ImGui by default and enables UI code paths when `AXIOM_ENABLE_IMGUI` is provided.

### Profiling hooks

A Tracy-style scope interface is provided via `ScopedZone` and macros (`AXIOM_PROFILE_FUNCTION`, `AXIOM_PROFILE_ZONE`, `AXIOM_PROFILE_FRAME_MARK`). Backends can be swapped at runtime by implementing `IProfilerBackend`.


### Asset + scene data scaffolds

A lightweight `AssetRegistry` introduces persistent handles, type metadata, and simple on-disk registry persistence. `SceneSerializer` provides the initial Save/Load seam for scene files so authored data can move out of hardcoded setup and into content files.

### Asset import pipeline scaffold

`AssetImporter` now provides a source-to-cooked pipeline pass. It scans source directories, infers asset types by extension, copies source files into a cooked output directory (`.axcooked` extension), computes source/cooked content hashes, and stores the mapping in `AssetRegistry` metadata.


### Scene authoring bootstrap

Runtime scene boot now attempts to load `Sandbox/scenes/default.axscene` through `SceneSerializer`; if missing, the engine creates a fallback cube entity and writes the file for future runs.


### Input action mapping scaffold

`InputSystem` now stores named actions (`SetActionState`, `IsPressed`, `Value`) so gameplay systems can consume semantic input (e.g., `RotateYaw`) instead of hardcoded key polling.

## Building the executable

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AxiomRuntime --config Release
```

Run the executable:

```bash
./build/AxiomRuntime
```


A Tracy-style scope interface is provided via `ScopedZone` and macros (`AXIOM_PROFILE_FUNCTION`, `AXIOM_PROFILE_ZONE`, `AXIOM_PROFILE_FRAME_MARK`). Backends can be swapped at runtime by implementing `IProfilerBackend`.

## Roadmap

### Near term (next 1-2 milestones)
- **Stabilize authored content flow:** expand `SceneSerializer` + `AssetRegistry` integration so scenes/prefabs reference stable asset handles and validate missing/deprecated assets at load.
- **Renderer quality baseline:** add material parameter support (albedo/normal/roughness/metallic), directional + point lights, and a basic shadow-map pass inside the current frame-graph abstraction.
- **Physics and gameplay iteration loop:** improve collision primitives beyond floor response (AABB/capsule + broadphase) and expose deterministic simulation controls in Sandbox for repeatable tests.
- **Lua scripting ergonomics:** move from `Update(entity, dt, position)` only to a richer API surface (transform, input actions, spawning, tags) with script error reporting surfaced in the editor layer.
- **Input action system completion:** add bindings, deadzone/axis curves, and serialized action maps so gameplay code no longer depends on raw key state.

### Mid term (engine maturity)
- **Job system backend:** implement a worker-thread pool backend for `IJobSystem`, then parallelize transform propagation, script updates, and render data extraction with deterministic fences.
- **Render architecture expansion:** evolve the frame graph from ordered pass execution to resource lifetime/transient allocation tracking, preparing deferred/compute-driven paths.
- **Asset pipeline v2:** add import recipes, dependency tracking, and incremental reimport (e.g., glTF + texture processing) so cooked output is reproducible and cacheable.
- **Editor tooling pass:** build core scene inspector/hierarchy, gizmos, and play-in-editor loop on top of `EditorLayer`, including profiling overlays from `ScopedZone` markers.
- **Prefab workflow:** support nested prefabs, override tracking, and prefab diff/apply operations for scalable content authoring.

### Long term (production readiness)
- **Streaming + large-world support:** asynchronous asset streaming, world partition/chunk loading, and memory budget enforcement for CPU/GPU resources.
- **Platform and rendering backends:** keep OpenGL path as reference while introducing an additional backend (e.g., Vulkan/Metal via abstraction seams already present in renderer/frame graph).
- **Networking foundation:** deterministic snapshot/rollback-friendly replication layer built around ECS component deltas.
- **Automated quality gates:** add CI for format/lint/build/tests, content validation checks, and performance regression tracking using profiling hooks.
- **Documentation and samples:** provide end-to-end sample projects (3D platformer/arena) that demonstrate ECS, scripting, assets, and editor workflows as canonical patterns.
