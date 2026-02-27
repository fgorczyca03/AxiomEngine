# Axiom Engine

A modular C++20 engine core with explicit module boundaries for long-term systems research.

## Folder Structure

- `AxiomEngine/core`: app loop, deterministic timing, lifetime control.
- `AxiomEngine/ecs`: archetype ECS with SoA component columns and signature-based archetypes.
- `AxiomEngine/scene`: transform hierarchy and world matrix propagation.
- `AxiomEngine/rendering`: OpenGL bootstrap, shader, mesh, camera, forward renderer.
- `AxiomEngine/physics`: minimal ECS-driven rigid body stepper.
- `AxiomEngine/assets`: asset caching interfaces.
- `AxiomEngine/scripting`: Lua bridge.
- `AxiomEngine/input`: input polling abstraction.
- `Sandbox`: sample application, shaders, and scripts.

## Architecture Highlights

### ECS

Entities are integral IDs. Components are assigned runtime type IDs and packed into archetype-local columns. Each archetype stores one SoA vector per component type in that archetype. Moving entities across signatures migrates row data between archetypes, preserving tight linear iteration for systems.

### Scene and Transform Propagation

Scene nodes carry parent IDs and transform components. Dirty local transforms recompute world transforms on update. This keeps simulation writes local and render reads consistent.

### Renderer

OpenGL 4.5 core profile via GLFW + GLAD. The renderer owns device objects (shader/mesh/camera), performs a forward pass, and samples ECS transform + mesh components.

### Physics

A minimal semi-implicit Euler solver updates rigid body velocity/position and syncs transform components directly, with simple floor collision response.

### Lua scripting

Lua `Update(entity, dt, position)` is called per entity each simulation tick. Script outputs are applied to ECS transform data.

### Parallelization readiness

Systems consume explicit component sets (`ForEach<...>`). No hidden globals and no singleton state mean each system can become a job that declares read/write component access, enabling later dependency-graph scheduling.
