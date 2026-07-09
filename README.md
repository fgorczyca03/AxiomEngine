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

### Prerequisites (Ubuntu/Debian)

Install the toolchain and Linux window-system/OpenGL development packages used by GLFW:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  libgl1-mesa-dev \
  libx11-dev \
  libxinerama-dev \
  libxcursor-dev \
  libxi-dev \
  libxrandr-dev \
  libwayland-dev \
  libxkbcommon-dev \
  wayland-protocols
```

From repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release --fresh
cmake --build build --target AxiomRuntime --config Release
```

Run:

```bash
./build/AxiomRuntime
```

## Desktop installation (Linux)

After building, install the runtime and desktop launcher with CMake:

```bash
cmake --install build --prefix "$HOME/.local"
```

This installs:

- `AxiomRuntime` to `$HOME/.local/bin`
- A desktop launcher to `$HOME/.local/share/applications/axiom-engine.desktop`
- A scalable launcher icon to `$HOME/.local/share/icons/hicolor/scalable/apps/axiom-engine.svg`

Most desktop environments discover those files automatically. If the launcher does not appear right away, update the desktop and icon caches:

```bash
update-desktop-database "$HOME/.local/share/applications" || true
gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" || true
```

For a system-wide install, use a system prefix instead:

```bash
sudo cmake --install build --prefix /usr/local
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
