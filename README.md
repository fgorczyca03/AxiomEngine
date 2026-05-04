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
- `AxiomEngine/scene`: transform hierarchy and world matrix propagation.
- `AxiomEngine/rendering`: OpenGL bootstrap, shader, mesh, camera, forward renderer, frame graph scaffold.
- `AxiomEngine/physics`: minimal ECS-driven rigid body stepper.
- `AxiomEngine/assets`: asset caching interfaces.
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
