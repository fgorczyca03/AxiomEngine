cmake_minimum_required(VERSION 3.23)
project(AxiomEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(AXIOM_BUILD_SANDBOX "Build sandbox executable" ON)
option(AXIOM_USE_BULLET "Use Bullet physics backend" OFF)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

include(FetchContent)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v2.0.6)
FetchContent_MakeAvailable(glad)

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
FetchContent_MakeAvailable(lua)

FetchContent_Declare(tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG v2.9.3)
FetchContent_MakeAvailable(tinygltf)

if(AXIOM_USE_BULLET)
  find_package(Bullet3 CONFIG REQUIRED)
endif()

add_library(AxiomEngine STATIC
  AxiomEngine/core/Application.cpp
  AxiomEngine/core/Time.cpp
  AxiomEngine/ecs/ECSWorld.cpp
  AxiomEngine/scene/SceneGraph.cpp
  AxiomEngine/rendering/Shader.cpp
  AxiomEngine/rendering/Mesh.cpp
  AxiomEngine/rendering/Renderer.cpp
  AxiomEngine/rendering/Camera.cpp
  AxiomEngine/physics/PhysicsSystem.cpp
  AxiomEngine/assets/AssetManager.cpp
  AxiomEngine/scripting/ScriptSystem.cpp
  AxiomEngine/input/InputSystem.cpp
)

target_include_directories(AxiomEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${tinygltf_SOURCE_DIR})

target_link_libraries(AxiomEngine
  PUBLIC
    glfw
    glad
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
  target_compile_definitions(AxiomSandbox PRIVATE AXIOM_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/Sandbox")
endif()
