cmake_minimum_required(VERSION 3.23)
project(AxiomEngine VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(AXIOM_BUILD_SANDBOX "Build sandbox executable" ON)
option(AXIOM_USE_BULLET "Use Bullet physics backend" OFF)

# NOTE: warning flags are intentionally NOT set globally here anymore.
# They are applied only to Axiom's own targets further down, so that
# third-party dependencies pulled in via FetchContent (GLFW, glad, sol2,
# Lua, tinygltf) are not compiled with -Wpedantic/-Wconversion/-permissive-,
# which they were never written to satisfy.
set(AXIOM_WARNING_FLAGS
  $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Wconversion>
)

include(FetchContent)
include(GNUInstallDirs)

if(UNIX AND NOT APPLE)
  option(AXIOM_GLFW_USE_WAYLAND "Build GLFW with Wayland backend on Linux" ON)
  option(AXIOM_GLFW_USE_X11 "Build GLFW with X11 backend on Linux" ON)

  # Backward compatibility: if callers pass raw GLFW flags,
  # mirror them into AXIOM-prefixed options.
  if(DEFINED GLFW_BUILD_WAYLAND)
    set(AXIOM_GLFW_USE_WAYLAND ${GLFW_BUILD_WAYLAND} CACHE BOOL "" FORCE)
  endif()
  if(DEFINED GLFW_BUILD_X11)
    set(AXIOM_GLFW_USE_X11 ${GLFW_BUILD_X11} CACHE BOOL "" FORCE)
  endif()

  if(NOT AXIOM_GLFW_USE_WAYLAND AND NOT AXIOM_GLFW_USE_X11)
    message(FATAL_ERROR "At least one Linux GLFW backend must be enabled (Wayland or X11).")
  endif()

  # Clear stale GLFW cache variable from older build trees and publish
  # backend selection using GLFW's current cache variable names.
  unset(GLFW_USE_WAYLAND CACHE)
  unset(GLFW_BUILD_WAYLAND CACHE)
  unset(GLFW_BUILD_X11 CACHE)

  set(GLFW_BUILD_WAYLAND ${AXIOM_GLFW_USE_WAYLAND} CACHE BOOL "" FORCE)
  set(GLFW_BUILD_X11 ${AXIOM_GLFW_USE_X11} CACHE BOOL "" FORCE)

  message(STATUS "Axiom GLFW backend: WAYLAND=${AXIOM_GLFW_USE_WAYLAND}, X11=${AXIOM_GLFW_USE_X11}")
  message(STATUS "Fedora/Nobara note: Wayland needs wayland-devel, wayland-protocols-devel, "
                  "libxkbcommon-devel. X11 needs libXrandr-devel, libXinerama-devel, "
                  "libXcursor-devel, libXi-devel, libXext-devel. Both also need "
                  "mesa-libGL-devel and libglvnd-devel.")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v2.0.6
  SOURCE_SUBDIR cmake)
FetchContent_MakeAvailable(glad)

# glad v2 exposes a code-generation function instead of a ready-made
# library target. Generate the OpenGL 4.5 core loader used by Renderer,
# Shader, and Mesh so clean build trees can link AxiomEngine directly.
# Requires Python3 on PATH at configure time (checked above).
glad_add_library(glad_gl_core_45 REPRODUCIBLE API gl:core=4.5)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.3.1)
FetchContent_MakeAvailable(sol2)

FetchContent_Declare(lua
  GIT_REPOSITORY https://github.com/lua/lua.git
  GIT_TAG v5.4.6)
FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
  FetchContent_Populate(lua)
endif()

# The official Lua repository intentionally does not ship a CMake project.
# Provide the embeddable interpreter library target expected by sol2.
add_library(lua STATIC
  ${lua_SOURCE_DIR}/lapi.c
  ${lua_SOURCE_DIR}/lcode.c
  ${lua_SOURCE_DIR}/lctype.c
  ${lua_SOURCE_DIR}/ldebug.c
  ${lua_SOURCE_DIR}/ldo.c
  ${lua_SOURCE_DIR}/ldump.c
  ${lua_SOURCE_DIR}/lfunc.c
  ${lua_SOURCE_DIR}/lgc.c
  ${lua_SOURCE_DIR}/llex.c
  ${lua_SOURCE_DIR}/lmem.c
  ${lua_SOURCE_DIR}/lobject.c
  ${lua_SOURCE_DIR}/lopcodes.c
  ${lua_SOURCE_DIR}/lparser.c
  ${lua_SOURCE_DIR}/lstate.c
  ${lua_SOURCE_DIR}/lstring.c
  ${lua_SOURCE_DIR}/ltable.c
  ${lua_SOURCE_DIR}/ltm.c
  ${lua_SOURCE_DIR}/lundump.c
  ${lua_SOURCE_DIR}/lvm.c
  ${lua_SOURCE_DIR}/lzio.c
  ${lua_SOURCE_DIR}/lauxlib.c
  ${lua_SOURCE_DIR}/lbaselib.c
  ${lua_SOURCE_DIR}/lcorolib.c
  ${lua_SOURCE_DIR}/ldblib.c
  ${lua_SOURCE_DIR}/liolib.c
  ${lua_SOURCE_DIR}/lmathlib.c
  ${lua_SOURCE_DIR}/loadlib.c
  ${lua_SOURCE_DIR}/loslib.c
  ${lua_SOURCE_DIR}/lstrlib.c
  ${lua_SOURCE_DIR}/ltablib.c
  ${lua_SOURCE_DIR}/lutf8lib.c
  ${lua_SOURCE_DIR}/linit.c
)
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})
target_compile_definitions(lua PUBLIC LUA_COMPAT_5_3)

FetchContent_Declare(tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG v2.9.3)
FetchContent_MakeAvailable(tinygltf)

if(AXIOM_USE_BULLET)
  find_package(Bullet3 CONFIG REQUIRED)
endif()

add_library(AxiomEngine STATIC
  AxiomEngine/core/Application.cpp
  AxiomEngine/core/JobSystem.cpp
  AxiomEngine/core/Time.cpp
  AxiomEngine/editor/EditorLayer.cpp
  AxiomEngine/ecs/ECSWorld.cpp
  AxiomEngine/scene/SceneGraph.cpp
  AxiomEngine/scene/SceneSerializer.cpp
  AxiomEngine/scene/Prefab.cpp
  AxiomEngine/rendering/Shader.cpp
  AxiomEngine/rendering/Mesh.cpp
  AxiomEngine/rendering/FrameGraph.cpp
  AxiomEngine/rendering/Renderer.cpp
  AxiomEngine/rendering/Camera.cpp
  AxiomEngine/profiling/Profiling.cpp
  AxiomEngine/physics/PhysicsSystem.cpp
  AxiomEngine/assets/AssetManager.cpp
  AxiomEngine/assets/AssetImporter.cpp
  AxiomEngine/assets/AssetRegistry.cpp
  AxiomEngine/scripting/ScriptSystem.cpp
  AxiomEngine/input/InputSystem.cpp
)

target_include_directories(AxiomEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${tinygltf_SOURCE_DIR})

# Prevent stb_image.h from being included multiple times in the same translation unit
target_compile_definitions(AxiomEngine PUBLIC STB_IMAGE_STATIC)
target_compile_definitions(AxiomEngine PRIVATE STB_IMAGE_IMPLEMENTATION)

target_compile_options(AxiomEngine PRIVATE ${AXIOM_WARNING_FLAGS})

target_link_libraries(AxiomEngine
  PUBLIC
    glfw
    glad_gl_core_45
    glm
    sol2
    lua
)

if(AXIOM_USE_BULLET)
  target_link_libraries(AxiomEngine PUBLIC Bullet3::BulletDynamics Bullet3::BulletCollision Bullet3::LinearMath)
  target_compile_definitions(AxiomEngine PUBLIC AXIOM_USE_BULLET=1)
endif()

if(AXIOM_BUILD_SANDBOX)
  add_executable(AxiomSandbox Sandbox/main.cpp)
  target_link_libraries(AxiomSandbox PRIVATE AxiomEngine)
  target_compile_options(AxiomSandbox PRIVATE ${AXIOM_WARNING_FLAGS})
  target_compile_definitions(AxiomSandbox PRIVATE AXIOM_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/Sandbox")
endif()

add_executable(AxiomRuntime App/main.cpp)
target_link_libraries(AxiomRuntime PRIVATE AxiomEngine)
target_compile_options(AxiomRuntime PRIVATE ${AXIOM_WARNING_FLAGS})

install(TARGETS AxiomRuntime
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/axiom-engine.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/packaging/linux/axiom-engine.desktop
    @ONLY
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/packaging/linux/axiom-engine.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
  )

  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/hicolor/scalable/apps/axiom-engine.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
  )
endif()
