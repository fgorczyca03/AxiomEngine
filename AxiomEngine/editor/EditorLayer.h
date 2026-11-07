#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Components.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct GLFWwindow;

namespace axiom::scene {
class SceneSerializer;
}

namespace axiom::rendering {
class Renderer;
}

namespace axiom::input {
class InputReplay;
class InputSystem;
}

namespace axiom::editor {

struct EditorFrameInfo {
    float deltaSeconds{0.0F};
};

class EditorLayer {
  private:
    enum class GizmoMode : unsigned char { Translate, Rotate, Scale };

    struct CachedAssetEntry {
        std::filesystem::path path{};
        std::string relativePath{};
        std::string filename{};
        std::string lowerFilename{};
        bool isDirectory{false};
    };

    struct CachedAssetDirectory {
        std::vector<CachedAssetEntry> entries{};
        std::chrono::steady_clock::time_point refreshAfter{};
    };

  public:
    bool Initialize(GLFWwindow* window, std::filesystem::path assetRoot);
    void Shutdown();

    void BeginFrame();
    void Draw(const EditorFrameInfo& frameInfo, ecs::ECSWorld& world, scene::SceneSerializer& serializer,
              unsigned int sceneTexture, rendering::Renderer& renderer, input::InputSystem& input,
              input::InputReplay& inputReplay);
    void EndFrame();

    [[nodiscard]] int SceneViewportWidth() const;
    [[nodiscard]] int SceneViewportHeight() const;
    [[nodiscard]] bool WantsKeyboardCapture() const;
    [[nodiscard]] bool WantsMouseCapture() const;

  private:
    void BuildDefaultLayout(unsigned int dockspaceId, float width, float height);
    void DrawHierarchy(ecs::ECSWorld& world);
    void DrawInspector(ecs::ECSWorld& world);
    void DrawScene(ecs::ECSWorld& world, unsigned int sceneTexture, rendering::Renderer& renderer, float deltaSeconds);
    void DrawProjectBrowser();
    void DrawAssetDirectory(const std::filesystem::path& directory, const std::string& filter);
    [[nodiscard]] const std::vector<std::string>& AvailableLuaScripts() const;
    void DrawAssetDirectory(const std::filesystem::path& directory);
    void DrawDebugOverlay(const EditorFrameInfo& frameInfo, scene::SceneSerializer& serializer,
                          input::InputSystem& input, input::InputReplay& inputReplay);
    [[nodiscard]] std::vector<std::string> AvailableLuaScripts() const;
    void CreateLuaScript();
    void DrawConsole();
    void CreatePrimitive(ecs::ECSWorld& world, rendering::PrimitiveShape shape);
    void DeleteSelectedEntity(ecs::ECSWorld& world);
    bool SaveScene(ecs::ECSWorld& world, scene::SceneSerializer& serializer);
    void Log(std::string message);

    GLFWwindow* window_{nullptr};
    std::filesystem::path assetRoot_{};
    std::filesystem::path scenePath_{};
    std::string settingsPath_{};
    // Selection and dock layout are editor state; entity data remains in the ECS world.
    ecs::Entity selectedEntity_{0};
    std::vector<std::string> consoleMessages_{};
    std::unordered_map<std::string, CachedAssetDirectory> assetDirectoryCache_{};
    mutable std::vector<std::string> availableLuaScripts_{};
    mutable std::chrono::steady_clock::time_point refreshScriptsAfter_{};
    std::string selectedAsset_{};
    std::string scriptEditorAsset_{};
    std::string saveStatus_{};
    std::array<char, 128> assetFilter_{};
    std::array<char, 64> newScriptName_{};
    std::array<char, 32768> scriptEditorBuffer_{};
    std::array<float, 60> frameTimeHistory_{};
    int sceneViewportWidth_{1280};
    int sceneViewportHeight_{720};
    std::size_t frameTimeCount_{0};
    std::size_t frameTimeCursor_{0};
    bool showHierarchy_{true};
    bool showInspector_{true};
    bool showScene_{true};
    bool showProject_{true};
    bool showConsole_{false};
    bool showDebugOverlay_{false};
    bool deleteSelected_{false};
    bool scriptEditorDirty_{false};
    bool scriptEditorTooLarge_{false};
    bool sceneNavigationActive_{false};
    GizmoMode gizmoMode_{GizmoMode::Translate};
    bool gizmoDragging_{false};
    int gizmoActiveAxis_{-1};
    glm::vec2 gizmoDragStartMouse_{0.0F};
    glm::vec2 gizmoDragScreenAxis_{0.0F};
    glm::vec3 gizmoDragAxisLocal_{0.0F};
    glm::vec3 gizmoDragAxisWorld_{0.0F};
    glm::vec3 gizmoDragStartTranslation_{0.0F};
    glm::quat gizmoDragStartRotation_{1.0F, 0.0F, 0.0F, 0.0F};
    glm::vec3 gizmoDragStartScale_{1.0F};
    float gizmoWorldPerPixel_{0.0F};
    float gizmoRotationStartAngle_{0.0F};
    bool initialized_{false};
};

} // namespace axiom::editor
