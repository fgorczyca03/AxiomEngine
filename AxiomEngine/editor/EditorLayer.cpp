#include <glad/gl.h> // MUST be first!

#include "AxiomEngine/editor/EditorLayer.h"

#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/scene/SceneSerializer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <system_error>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::editor {

bool EditorLayer::Initialize(GLFWwindow* window, std::filesystem::path assetRoot) {
    AXIOM_PROFILE_FUNCTION();
    if (window == nullptr) {
        return false;
    }

    window_ = window;
    assetRoot_ = std::move(assetRoot);
    scenePath_ = assetRoot_ / "scenes" / "default.axscene";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    settingsPath_ = (assetRoot_.parent_path() / "AxiomEditor.ini").string();
    io.IniFilename = settingsPath_.c_str();

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0F;
    style.FrameRounding = 3.0F;
    style.GrabRounding = 3.0F;
    style.TabRounding = 3.0F;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.105F, 0.112F, 0.145F, 1.0F);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20F, 0.32F, 0.43F, 0.72F);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26F, 0.43F, 0.56F, 0.86F);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.25F, 0.47F, 0.63F, 1.0F);

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        ImGui::DestroyContext();
        window_ = nullptr;
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 450 core")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        window_ = nullptr;
        return false;
    }

    initialized_ = true;
    Log("Editor ready. Add a shape from the Hierarchy panel or GameObject menu.");
    return true;
}

void EditorLayer::Shutdown() {
    AXIOM_PROFILE_FUNCTION();
    if (!initialized_) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
    window_ = nullptr;
}

void EditorLayer::BeginFrame() {
    if (!initialized_) {
        return;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorLayer::Draw(const EditorFrameInfo& frameInfo, ecs::ECSWorld& world, scene::SceneSerializer& serializer,
                       unsigned int sceneTexture) {
    if (!initialized_) {
        return;
    }

    bool requestSave = false;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    constexpr ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                           ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;
    ImGui::Begin("Axiom Editor Host", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            requestSave = ImGui::MenuItem("Save Scene", "Ctrl+S");
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::BeginMenu("Create Shape")) {
                for (std::size_t i = 0; i < rendering::PrimitiveShapeNames.size(); ++i) {
                    if (ImGui::MenuItem(rendering::PrimitiveShapeNames[i].data())) {
                        CreatePrimitive(world, static_cast<rendering::PrimitiveShape>(i));
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy_);
            ImGui::MenuItem("Inspector", nullptr, &showInspector_);
            ImGui::MenuItem("Scene", nullptr, &showScene_);
            ImGui::MenuItem("Project", nullptr, &showProject_);
            ImGui::MenuItem("Console", nullptr, &showConsole_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("AxiomEditorDockspace");
    const bool needsDefaultLayout = ImGui::DockBuilderGetNode(dockspaceId) == nullptr;
    const ImVec2 dockspaceSize = ImGui::GetContentRegionAvail();
    ImGui::DockSpace(dockspaceId, dockspaceSize, ImGuiDockNodeFlags_PassthruCentralNode);
    if (needsDefaultLayout) {
        BuildDefaultLayout(dockspaceId, dockspaceSize.x, dockspaceSize.y);
    }
    ImGui::End();

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        requestSave = true;
    }
    if (requestSave) {
        SaveScene(world, serializer);
    }

    if (showHierarchy_) {
        DrawHierarchy(world);
    }
    if (showScene_) {
        DrawScene(sceneTexture);
    }
    if (showInspector_) {
        DrawInspector(world);
    }
    if (showProject_) {
        DrawProjectBrowser();
    }
    if (showConsole_) {
        DrawConsole();
    }

    if (deleteSelected_) {
        DeleteSelectedEntity(world);
        deleteSelected_ = false;
    }

    if (!saveStatus_.empty()) {
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 12.0F, viewport->Pos.y + viewport->Size.y - 36.0F),
                                ImGuiCond_Always);
        ImGui::Begin("##EditorStatus", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav);
        ImGui::TextUnformatted(saveStatus_.c_str());
        ImGui::End();
    }

    (void)frameInfo;
}

void EditorLayer::EndFrame() {
    if (!initialized_) {
        return;
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int EditorLayer::SceneViewportWidth() const { return sceneViewportWidth_; }

int EditorLayer::SceneViewportHeight() const { return sceneViewportHeight_; }

bool EditorLayer::WantsKeyboardCapture() const { return initialized_ && ImGui::GetIO().WantCaptureKeyboard; }

bool EditorLayer::WantsMouseCapture() const { return initialized_ && ImGui::GetIO().WantCaptureMouse; }

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceId, float width, float height) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImVec2(width, height));

    ImGuiID topId = dockspaceId;
    ImGuiID projectId = 0;
    ImGui::DockBuilderSplitNode(topId, ImGuiDir_Down, 0.24F, &projectId, &topId);

    ImGuiID hierarchyId = 0;
    ImGuiID centerRightId = topId;
    ImGui::DockBuilderSplitNode(centerRightId, ImGuiDir_Left, 0.20F, &hierarchyId, &centerRightId);

    ImGuiID inspectorId = 0;
    ImGuiID sceneId = centerRightId;
    ImGui::DockBuilderSplitNode(sceneId, ImGuiDir_Right, 0.24F, &inspectorId, &sceneId);

    ImGui::DockBuilderDockWindow("Hierarchy", hierarchyId);
    ImGui::DockBuilderDockWindow("Inspector", inspectorId);
    ImGui::DockBuilderDockWindow("Scene", sceneId);
    ImGui::DockBuilderDockWindow("Project", projectId);
    ImGui::DockBuilderDockWindow("Console", projectId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayer::DrawHierarchy(ecs::ECSWorld& world) {
    if (!ImGui::Begin("Hierarchy", &showHierarchy_)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("+ Add Shape", ImVec2(-1.0F, 0.0F))) {
        ImGui::OpenPopup("Add Shape");
    }
    if (ImGui::BeginPopup("Add Shape")) {
        for (std::size_t i = 0; i < rendering::PrimitiveShapeNames.size(); ++i) {
            if (ImGui::MenuItem(rendering::PrimitiveShapeNames[i].data())) {
                CreatePrimitive(world, static_cast<rendering::PrimitiveShape>(i));
            }
        }
        ImGui::EndPopup();
    }
    ImGui::Separator();

    std::vector<ecs::Entity> entities{};
    world.ForEach<scene::TransformComponent, scene::SceneNodeComponent, rendering::MeshComponent>(
        [&](ecs::Entity entity, const scene::TransformComponent&, const scene::SceneNodeComponent&,
            const rendering::MeshComponent&) { entities.push_back(entity); });
    std::sort(entities.begin(), entities.end());
    if (selectedEntity_ == 0U && !entities.empty()) {
        selectedEntity_ = entities.front();
    }

    for (const ecs::Entity entity : entities) {
        const auto named = std::find_if(entityNames_.begin(), entityNames_.end(),
                                        [entity](const auto& entry) { return entry.first == entity; });
        const std::string label = named == entityNames_.end() ? "Entity " + std::to_string(entity) : named->second;
        ImGui::PushID(static_cast<int>(entity));
        if (ImGui::Selectable(label.c_str(), selectedEntity_ == entity)) {
            selectedEntity_ = entity;
        }
        ImGui::PopID();
    }

    if (entities.empty()) {
        ImGui::TextDisabled("No scene objects yet.");
    }
    ImGui::End();
}

void EditorLayer::DrawInspector(ecs::ECSWorld& world) {
    if (!ImGui::Begin("Inspector", &showInspector_)) {
        ImGui::End();
        return;
    }

    if (selectedEntity_ == 0U || !world.IsAlive(selectedEntity_)) {
        ImGui::TextDisabled("Select an object in the Hierarchy.");
        ImGui::End();
        return;
    }

    ImGui::Text("Entity %u", selectedEntity_);
    ImGui::Separator();
    if (auto* transform = world.GetComponent<scene::TransformComponent>(selectedEntity_)) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool changed = ImGui::DragFloat3("Position", &transform->local.translation.x, 0.05F);
            glm::vec3 rotationDegrees = glm::degrees(glm::eulerAngles(transform->local.rotation));
            if (ImGui::DragFloat3("Rotation", &rotationDegrees.x, 0.5F)) {
                transform->local.rotation = glm::normalize(glm::quat(glm::radians(rotationDegrees)));
                changed = true;
            }
            changed = ImGui::DragFloat3("Scale", &transform->local.scale.x, 0.05F) || changed;
            if (changed) {
                transform->dirty = true;
            }
        }
    }

    if (auto* mesh = world.GetComponent<rendering::MeshComponent>(selectedEntity_)) {
        if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            const std::string_view currentShape = rendering::PrimitiveShapeName(mesh->primitive);
            if (ImGui::BeginCombo("Shape", currentShape.data())) {
                for (std::size_t i = 0; i < rendering::PrimitiveShapeNames.size(); ++i) {
                    const auto shape = static_cast<rendering::PrimitiveShape>(i);
                    const bool selected = mesh->primitive == shape;
                    if (ImGui::Selectable(rendering::PrimitiveShapeNames[i].data(), selected)) {
                        mesh->primitive = shape;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text("Mesh handle: %u", mesh->meshHandle);
            ImGui::Text("Material handle: %u", mesh->materialHandle);
            ImGui::TextDisabled("Built-in primitive geometry");
        }
    }
    if (auto* body = world.GetComponent<physics::RigidBodyComponent>(selectedEntity_)) {
        if (ImGui::CollapsingHeader("Rigid Body")) {
            ImGui::DragFloat("Mass", &body->mass, 0.05F, 0.0F, 10000.0F);
            ImGui::DragFloat3("Half Extent", &body->halfExtent.x, 0.05F, 0.0F, 1000.0F);
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("Delete Entity", ImVec2(-1.0F, 0.0F))) {
        deleteSelected_ = true;
    }
    ImGui::End();
}

void EditorLayer::DrawScene(unsigned int sceneTexture) {
    if (!ImGui::Begin("Scene", &showScene_)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Scene View   |   Drag panels to rearrange   |   Edit objects in the Inspector");
    ImGui::Separator();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
    sceneViewportWidth_ = std::max(1, static_cast<int>(std::lround(available.x * framebufferScale.x)));
    sceneViewportHeight_ = std::max(1, static_cast<int>(std::lround(available.y * framebufferScale.y)));

    if (sceneTexture == 0U || available.x <= 1.0F || available.y <= 1.0F) {
        ImGui::TextDisabled("Waiting for the scene renderer...");
        ImGui::End();
        return;
    }

    const float imageAspect = static_cast<float>(sceneViewportWidth_) / static_cast<float>(sceneViewportHeight_);
    const float availableAspect = available.x / available.y;
    ImVec2 imageSize = available;
    if (availableAspect > imageAspect) {
        imageSize.x = available.y * imageAspect;
    } else {
        imageSize.y = available.x / imageAspect;
    }
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursor.x + (available.x - imageSize.x) * 0.5F,
                               cursor.y + (available.y - imageSize.y) * 0.5F));
    ImGui::Image(ImTextureRef(static_cast<ImTextureID>(sceneTexture)), imageSize, ImVec2(0.0F, 1.0F),
                 ImVec2(1.0F, 0.0F));
    ImGui::End();
}

void EditorLayer::DrawProjectBrowser() {
    if (!ImGui::Begin("Project", &showProject_)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("PROJECT CONTENTS");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputTextWithHint("##AssetFilter", "Filter assets", assetFilter_, sizeof(assetFilter_));
    ImGui::Separator();

    std::error_code error;
    if (std::filesystem::is_directory(assetRoot_, error)) {
        DrawAssetDirectory(assetRoot_);
    } else {
        ImGui::TextDisabled("Project content folder is unavailable:");
        ImGui::TextWrapped("%s", assetRoot_.string().c_str());
    }

    if (!selectedAsset_.empty()) {
        ImGui::Separator();
        const std::filesystem::path selectedPath = assetRoot_ / selectedAsset_;
        ImGui::Text("Selected: %s", selectedPath.filename().string().c_str());
        ImGui::TextWrapped("%s", selectedPath.string().c_str());
        error.clear();
        if (std::filesystem::is_regular_file(selectedPath, error)) {
            error.clear();
            const auto bytes = std::filesystem::file_size(selectedPath, error);
            if (!error) {
                ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(bytes));
            }
            ImGui::TextDisabled("Preview is not available for this file type yet.");
        }
    }
    ImGui::End();
}

void EditorLayer::DrawAssetDirectory(const std::filesystem::path& directory) {
    std::error_code error;
    std::vector<std::filesystem::directory_entry> entries{};
    for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
        entries.push_back(*it);
    }
    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
        const bool leftDirectory = left.is_directory();
        const bool rightDirectory = right.is_directory();
        if (leftDirectory != rightDirectory) {
            return leftDirectory;
        }
        return left.path().filename().string() < right.path().filename().string();
    });

    std::string filter = assetFilter_;
    std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& entry : entries) {
        const std::string relative = entry.path().lexically_relative(assetRoot_).generic_string();
        std::error_code entryError;
        if (entry.is_directory(entryError)) {
            const std::string label = "[+] " + entry.path().filename().string();
            if (ImGui::TreeNodeEx(relative.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label.c_str())) {
                DrawAssetDirectory(entry.path());
                ImGui::TreePop();
            }
            continue;
        }

        std::string filename = entry.path().filename().string();
        std::string lowerFilename = filename;
        std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!filter.empty() && lowerFilename.find(filter) == std::string::npos && relative.find(filter) == std::string::npos) {
            continue;
        }

        const std::string label = "[file] " + filename;
        if (ImGui::Selectable(label.c_str(), selectedAsset_ == relative)) {
            selectedAsset_ = relative;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", entry.path().string().c_str());
        }
    }
}

void EditorLayer::DrawConsole() {
    if (!ImGui::Begin("Console", &showConsole_)) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("Clear")) {
        consoleMessages_.clear();
    }
    ImGui::Separator();
    for (const auto& message : consoleMessages_) {
        ImGui::TextUnformatted(message.c_str());
    }
    ImGui::End();
}

void EditorLayer::CreatePrimitive(ecs::ECSWorld& world, rendering::PrimitiveShape shape) {
    const ecs::Entity entity = world.CreateEntity();
    scene::TransformComponent transform{};
    std::size_t objectCount = 0;
    world.ForEach<scene::TransformComponent, rendering::MeshComponent>(
        [&](ecs::Entity, const scene::TransformComponent&, const rendering::MeshComponent&) { ++objectCount; });
    transform.local.translation.x = static_cast<float>(objectCount % 5U) * 1.5F;
    transform.local.translation.z = static_cast<float>(objectCount / 5U) * 1.5F;
    scene::SceneNodeComponent node{};
    rendering::MeshComponent mesh{1U, 2U, shape};
    physics::RigidBodyComponent body{};
    world.AddComponent(entity, transform);
    world.AddComponent(entity, node);
    world.AddComponent(entity, mesh);
    world.AddComponent(entity, body);
    selectedEntity_ = entity;
    const std::string name{rendering::PrimitiveShapeName(shape)};
    entityNames_.emplace_back(entity, name + " " + std::to_string(objectCount + 1U));
    Log("Created " + name + " " + std::to_string(objectCount + 1U) + ".");
}

void EditorLayer::DeleteSelectedEntity(ecs::ECSWorld& world) {
    if (selectedEntity_ == 0U || !world.IsAlive(selectedEntity_)) {
        return;
    }

    ecs::Entity parent = 0U;
    if (const auto* node = world.GetComponent<scene::SceneNodeComponent>(selectedEntity_)) {
        parent = node->parent;
    }
    world.ForEach<scene::SceneNodeComponent, scene::TransformComponent>(
        [&](ecs::Entity entity, scene::SceneNodeComponent& node, scene::TransformComponent& transform) {
            if (entity != selectedEntity_ && node.parent == selectedEntity_) {
                node.parent = parent;
                transform.dirty = true;
            }
        });

    const ecs::Entity removed = selectedEntity_;
    world.DestroyEntity(removed);
    entityNames_.erase(std::remove_if(entityNames_.begin(), entityNames_.end(),
                                      [removed](const auto& entry) { return entry.first == removed; }),
                       entityNames_.end());
    selectedEntity_ = 0U;
    Log("Deleted Entity " + std::to_string(removed) + ".");
}

bool EditorLayer::SaveScene(ecs::ECSWorld& world, scene::SceneSerializer& serializer) {
    const bool saved = serializer.Save(world, scenePath_.string());
    saveStatus_ = saved ? "Scene saved" : "Scene save failed";
    Log(saveStatus_ + ": " + scenePath_.string());
    return saved;
}

void EditorLayer::Log(std::string message) {
    consoleMessages_.push_back(std::move(message));
    constexpr std::size_t maxEntries = 200;
    if (consoleMessages_.size() > maxEntries) {
        consoleMessages_.erase(consoleMessages_.begin(), consoleMessages_.begin() +
                                                          static_cast<std::ptrdiff_t>(consoleMessages_.size() - maxEntries));
    }
}

} // namespace axiom::editor
