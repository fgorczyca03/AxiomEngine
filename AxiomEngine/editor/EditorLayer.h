#pragma once

#include "AxiomEngine/ecs/ECSWorld.h"
#include "AxiomEngine/rendering/Components.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
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
  private:
    enum class GizmoMode : unsigned char { Translate, Rotate, Scale };

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
    void DrawScene(ecs::ECSWorld& world, unsigned int sceneTexture, rendering::Renderer& renderer, float deltaSeconds);
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
