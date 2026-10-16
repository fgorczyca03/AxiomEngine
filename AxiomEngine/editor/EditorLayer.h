#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Components.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

struct GLFWwindow;

namespace axiom::scene {
class SceneSerializer;
}

namespace axiom::rendering {
class Renderer;
}

namespace axiom::editor {

struct EditorFrameInfo {
    float deltaSeconds{0.0F};
};

class EditorLayer {
  public:
    bool Initialize(GLFWwindow* window, std::filesystem::path assetRoot);
    void Shutdown();

    void BeginFrame();
    void Draw(const EditorFrameInfo& frameInfo, ecs::ECSWorld& world, scene::SceneSerializer& serializer,
              unsigned int sceneTexture, rendering::Renderer& renderer);
    void EndFrame();

    [[nodiscard]] int SceneViewportWidth() const;
    [[nodiscard]] int SceneViewportHeight() const;
    [[nodiscard]] bool WantsKeyboardCapture() const;
    [[nodiscard]] bool WantsMouseCapture() const;

  private:
    void BuildDefaultLayout(unsigned int dockspaceId, float width, float height);
    void DrawHierarchy(ecs::ECSWorld& world);
    void DrawInspector(ecs::ECSWorld& world);
    void DrawScene(unsigned int sceneTexture, rendering::Renderer& renderer, float deltaSeconds);
    void DrawProjectBrowser();
    void DrawAssetDirectory(const std::filesystem::path& directory);
    void DrawConsole();
    void CreatePrimitive(ecs::ECSWorld& world, rendering::PrimitiveShape shape);
    void DeleteSelectedEntity(ecs::ECSWorld& world);
    bool SaveScene(ecs::ECSWorld& world, scene::SceneSerializer& serializer);
    void Log(std::string message);

    GLFWwindow* window_{nullptr};
    std::filesystem::path assetRoot_{};
    std::filesystem::path scenePath_{};
    std::string settingsPath_{};
    ecs::Entity selectedEntity_{0};
    std::vector<std::pair<ecs::Entity, std::string>> entityNames_{};
    std::vector<std::string> consoleMessages_{};
    std::string selectedAsset_{};
    std::string saveStatus_{};
    char assetFilter_[128]{};
    int sceneViewportWidth_{1280};
    int sceneViewportHeight_{720};
    bool showHierarchy_{true};
    bool showInspector_{true};
    bool showScene_{true};
    bool showProject_{true};
    bool showConsole_{false};
    bool deleteSelected_{false};
    bool sceneNavigationActive_{false};
    bool initialized_{false};
};

} // namespace axiom::editor
