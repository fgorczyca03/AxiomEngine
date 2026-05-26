Axiom Engine Coding Style
=========================

This document captures the coding conventions already reflected in the codebase.

Core principles
---------------

- Prefer explicit, readable code over clever abstractions.
- Keep subsystem boundaries clear (core, ECS, scene, rendering, physics, tooling).
- Favor deterministic runtime behavior for simulation-facing code.

C++ language and compiler expectations
--------------------------------------

- Use **C++20**.
- Keep code warning-clean under:

  - ``/W4 /permissive-`` (MSVC)
  - ``-Wall -Wextra -Wpedantic -Wconversion`` (GCC/Clang)

- Avoid compiler-specific extensions unless wrapped behind clear platform guards.

Naming conventions
------------------

- **Types**: ``PascalCase`` (e.g., ``Application``, ``SceneGraph``).
- **Functions/methods**: ``PascalCase`` for engine APIs (e.g., ``Initialize``, ``BeginFrame``).
- **Variables/fields**: ``camelCase`` or trailing-underscore member fields when already used in module style.
- **Macros/compile definitions**: ``UPPER_SNAKE_CASE`` (e.g., ``AXIOM_PROFILE_FUNCTION``).

Formatting and structure
------------------------

- Use consistent indentation and brace style within each file.
- Prefer short, focused functions.
- Place public API declarations before private helpers.
- Include only what you use; avoid broad includes when forward declarations are sufficient.

ECS and data-oriented guidance
------------------------------

- Keep hot-path iteration linear and cache-friendly.
- Prefer contiguous storage and explicit migration semantics.
- Avoid hidden global state in systems; pass dependencies explicitly.

Error handling and diagnostics
------------------------------

- Fail fast for invalid engine configuration (e.g., contradictory backend options).
- Provide precise error messages for build/runtime setup issues.
- Use assertions and profiling hooks where they improve debuggability without obscuring logic.

Build and dependency conventions
--------------------------------

- Keep build options explicit and documented.
- Maintain backward-compatible option shims when deprecating external configuration flags.
- Ensure CI starts from a fresh CMake configuration state.

Documentation conventions
-------------------------

- Keep docs practical and task-oriented.
- Update README/docs when public behavior, build flags, or developer workflows change.
- Prefer concise examples that match current CI/build commands.
