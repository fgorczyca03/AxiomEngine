#include <glad/gl.h> // MUST be first!

#include "AxiomEngine/editor/EditorLayer.h"

#include "AxiomEngine/input/InputReplay.h"
#include "AxiomEngine/input/InputSystem.h"
#include "AxiomEngine/physics/Components.h"
#include "AxiomEngine/profiling/Profiling.h"
#include "AxiomEngine/rendering/Components.h"
#include "AxiomEngine/rendering/Renderer.h"
#include "AxiomEngine/scene/Components.h"
#include "AxiomEngine/scene/SceneSerializer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <system_error>

#ifndef AXIOM_ASSET_ROOT
#define AXIOM_ASSET_ROOT "."
#endif

namespace axiom::editor {

namespace {

bool ProjectToScreen(const glm::mat4& viewProjection, const glm::vec3& position, const ImVec2& imageMin,
                     const ImVec2& imageSize, ImVec2& screenPosition) {
    const glm::vec4 clip = viewProjection * glm::vec4(position, 1.0F);
    // Points behind the camera cannot be used for gizmo hit-testing.
    if (clip.w <= 0.0001F) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y) || !std::isfinite(ndc.z)) {
        return false;
    }
    screenPosition = ImVec2(imageMin.x + (ndc.x + 1.0F) * 0.5F * imageSize.x,
                            imageMin.y + (1.0F - ndc.y) * 0.5F * imageSize.y);
    return true;
}

float DistanceToSegment(const ImVec2& point, const ImVec2& start, const ImVec2& end, float* fraction = nullptr) {
    const glm::vec2 direction{end.x - start.x, end.y - start.y};
    const glm::vec2 offset{point.x - start.x, point.y - start.y};
    const float lengthSquared = glm::dot(direction, direction);
    // Clamp to the drawn segment so clicking near an axis extension does not grab the handle.
    const float t = lengthSquared > 0.0001F ? std::clamp(glm::dot(offset, direction) / lengthSquared, 0.0F, 1.0F)
                                           : 0.0F;
    if (fraction != nullptr) {
        *fraction = t;
    }
    const glm::vec2 closest = glm::vec2(start.x, start.y) + direction * t;
    return glm::length(glm::vec2(point.x, point.y) - closest);
}

const char* BindingTypeName(input::InputBindingType type) {
    switch (type) {
    case input::InputBindingType::Key: return "Key";
    case input::InputBindingType::Axis: return "Key Axis";
    case input::InputBindingType::MouseAxis: return "Mouse Axis";
    case input::InputBindingType::GamepadAxis: return "Gamepad Axis";
    case input::InputBindingType::GamepadButton: return "Gamepad Button";
    }
    return "Unknown";
}

} // namespace

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
    // Dear ImGui persists dock positions outside the content folder, next to the project root.
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
    // If the GL backend fails, unwind the GLFW backend and context initialized above.
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
                       unsigned int sceneTexture, rendering::Renderer& renderer, input::InputSystem& inputSystem,
                       input::InputReplay& inputReplay) {
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
            ImGui::MenuItem("Debug Overlay", nullptr, &showDebugOverlay_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Build the starter layout only once; after that, let ImGui's saved layout win.
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
        DrawScene(world, sceneTexture, renderer, frameInfo.deltaSeconds);
    } else {
        sceneNavigationActive_ = false;
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
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
    if (showDebugOverlay_) {
        DrawDebugOverlay(frameInfo, serializer, inputSystem, inputReplay);
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

}

void EditorLayer::DrawDebugOverlay(const EditorFrameInfo& frameInfo, scene::SceneSerializer& serializer,
                                   input::InputSystem& inputSystem, input::InputReplay& inputReplay) {
    const float frameSeconds = std::max(frameInfo.deltaSeconds, 0.0F);
    // Reuse a small fixed buffer instead of allocating history on every editor frame.
    frameTimeHistory_[frameTimeCursor_] = frameSeconds;
    frameTimeCursor_ = (frameTimeCursor_ + 1U) % frameTimeHistory_.size();
    frameTimeCount_ = std::min(frameTimeCount_ + 1U, frameTimeHistory_.size());

    // Before the buffer fills, ignore its unused zero-initialized slots.
    float totalFrameSeconds = 0.0F;
    for (std::size_t i = 0; i < frameTimeCount_; ++i) {
        totalFrameSeconds += frameTimeHistory_[i];
    }
    const float averageFrameSeconds = frameTimeCount_ > 0U
                                          ? totalFrameSeconds / static_cast<float>(frameTimeCount_)
                                          : 0.0F;

    if (!ImGui::Begin("Debug Overlay", &showDebugOverlay_)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Frame: %.2f ms  |  %.1f FPS", frameSeconds * 1000.0F,
                frameSeconds > 0.000001F ? 1.0F / frameSeconds : 0.0F);
    ImGui::Text("Rolling average (%zu frames): %.2f ms", frameTimeCount_, averageFrameSeconds * 1000.0F);

    ImGui::Separator();
    // Show action state and bindings together so a strange value can be traced to its source.
    const std::vector<std::string> actionNames = inputSystem.ActionNames();
    const std::string actionMapLabel = "Input Action Map (" + std::to_string(actionNames.size()) + ")";
    if (ImGui::TreeNode(actionMapLabel.c_str())) {
        for (const std::string& actionName : actionNames) {
            ImGui::Text("%s: %+.3f%s%s", actionName.c_str(), inputSystem.Value(actionName),
                        inputSystem.IsPressed(actionName) ? "  pressed" : "",
                        inputSystem.WasPressed(actionName) ? "  just pressed" :
                            (inputSystem.WasReleased(actionName) ? "  just released" : ""));
            const std::string bindingsNode = "Bindings##" + actionName;
            if (ImGui::TreeNode(bindingsNode.c_str())) {
                for (const input::InputBindingDescription& binding : inputSystem.DescribeBindings(actionName)) {
                    if (binding.type == input::InputBindingType::Axis) {
                        ImGui::BulletText("%s: negative key %d, positive key %d, scale %.2f",
                                          BindingTypeName(binding.type), binding.negativeCode,
                                          binding.positiveCode, binding.scale);
                    } else {
                        ImGui::BulletText("%s: code %d, scale %.2f", BindingTypeName(binding.type),
                                          binding.positiveCode, binding.scale);
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    const auto& validationIssues = serializer.ValidationIssues();
    ImGui::Text("Scene validation: %zu warning%s", validationIssues.size(),
                validationIssues.size() == 1U ? "" : "s");
    if (validationIssues.empty()) {
        ImGui::TextDisabled("No scene validation warnings.");
    } else {
        for (const scene::SceneSerializer::ValidationIssue& issue : validationIssues) {
            ImGui::BulletText("Line %zu, entity %zu: %s", issue.lineNumber, issue.entityIndex,
                              issue.message.c_str());
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Fixed-step input replay");
    // Keep one obvious replay path for the first version; each new recording replaces the previous one.
    const std::string latestReplayPath = (assetRoot_ / "replays" / "latest.axreplay").string();
    ImGui::TextWrapped("File: %s", latestReplayPath.c_str());
    if (inputReplay.Mode() == input::InputReplayMode::Idle) {
        if (ImGui::Button("Record Input Replay")) {
            if (inputReplay.StartRecording(latestReplayPath)) {
                Log("Recording fixed-step input to " + latestReplayPath);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Latest Replay")) {
            if (inputReplay.StartPlayback(latestReplayPath)) {
                Log("Playing fixed-step input from " + latestReplayPath);
            }
        }
    } else if (inputReplay.Mode() == input::InputReplayMode::Recording) {
        ImGui::Text("Recording frame %llu", static_cast<unsigned long long>(inputReplay.FrameIndex()));
        if (ImGui::Button("Stop Recording")) {
            if (inputReplay.StopRecording()) {
                Log("Saved input replay to " + latestReplayPath);
            }
        }
    } else {
        ImGui::Text("Playback frame %llu", static_cast<unsigned long long>(inputReplay.FrameIndex()));
        if (ImGui::Button("Stop Playback")) {
            inputReplay.StopPlayback();
        }
    }
    if (!inputReplay.LastError().empty()) {
        ImGui::TextColored(ImVec4(1.0F, 0.38F, 0.35F, 1.0F), "%s", inputReplay.LastError().c_str());
    }
    ImGui::End();
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

    // Leave the center Scene view large, with the Project browser across the bottom.
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
    // ECS archetype order can change after component edits; keep the visible hierarchy predictable.
    std::sort(entities.begin(), entities.end());
    if (selectedEntity_ == 0U && !entities.empty()) {
        selectedEntity_ = entities.front();
    }

    for (const ecs::Entity entity : entities) {
        const auto* name = world.GetComponent<scene::NameComponent>(entity);
        const std::string label = name != nullptr && !name->value.empty() ? name->value : "Entity " + std::to_string(entity);
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
    if (auto* name = world.GetComponent<scene::NameComponent>(selectedEntity_)) {
        std::array<char, 128> nameBuffer{};
        const std::size_t copied = std::min(name->value.size(), nameBuffer.size() - 1U);
        std::copy_n(name->value.data(), copied, nameBuffer.data());
        if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
            name->value = nameBuffer.data();
        }
    }
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

    auto* script = world.GetComponent<scene::ScriptComponent>(selectedEntity_);
    if (script == nullptr && ImGui::Button("+ Add Script", ImVec2(-1.0F, 0.0F))) {
        world.AddComponent(selectedEntity_, scene::ScriptComponent{});
        script = world.GetComponent<scene::ScriptComponent>(selectedEntity_);
        Log("Added a script slot to Entity " + std::to_string(selectedEntity_) + ".");
    }
    bool removeScript = false;
    if (script != nullptr && ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled", &script->enabled);
        const std::vector<std::string> scripts = AvailableLuaScripts();
        const char* currentScript = script->scriptPath.empty() ? "Choose a Lua script..." : script->scriptPath.c_str();
        if (ImGui::BeginCombo("Source", currentScript)) {
            for (const std::string& path : scripts) {
                const bool selected = script->scriptPath == path;
                if (ImGui::Selectable(path.c_str(), selected)) {
                    script->scriptPath = path;
                    script->enabled = true;
                    script->lastError.clear();
                    Log("Attached " + path + " to Entity " + std::to_string(selectedEntity_) + ".");
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (script->scriptPath.empty()) {
            ImGui::TextDisabled("Create a Lua file in Project, then choose it here.");
        } else if (std::find(scripts.begin(), scripts.end(), script->scriptPath) == scripts.end()) {
            ImGui::TextColored(ImVec4(1.0F, 0.55F, 0.35F, 1.0F), "Script file is missing.");
        }
        if (!script->lastError.empty()) {
            ImGui::TextColored(ImVec4(1.0F, 0.45F, 0.4F, 1.0F), "%s", script->lastError.c_str());
        }
        if (ImGui::Button("Remove Script", ImVec2(-1.0F, 0.0F))) {
            removeScript = true;
        }
    }
    if (removeScript) {
        world.RemoveComponent<scene::ScriptComponent>(selectedEntity_);
        Log("Removed the script from Entity " + std::to_string(selectedEntity_) + ".");
    }

    ImGui::Spacing();
    if (ImGui::Button("Delete Entity", ImVec2(-1.0F, 0.0F))) {
        deleteSelected_ = true;
    }
    ImGui::End();
}

void EditorLayer::DrawScene(ecs::ECSWorld& world, unsigned int sceneTexture, rendering::Renderer& renderer,
                            float deltaSeconds) {
    if (!ImGui::Begin("Scene", &showScene_)) {
        sceneNavigationActive_ = false;
        gizmoDragging_ = false;
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Right-drag: look/fly  |  WASD: move  |  Q/E: up/down  |  MMB: pan  |  Wheel: zoom  |  Shift: faster");
    ImGui::TextDisabled("Transform:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Move [W]", gizmoMode_ == GizmoMode::Translate)) {
        gizmoMode_ = GizmoMode::Translate;
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate [E]", gizmoMode_ == GizmoMode::Rotate)) {
        gizmoMode_ = GizmoMode::Rotate;
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale [R]", gizmoMode_ == GizmoMode::Scale)) {
        gizmoMode_ = GizmoMode::Scale;
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
    }
    ImGui::SameLine();
    if (selectedEntity_ != 0U && world.IsAlive(selectedEntity_)) {
        ImGui::TextDisabled("Entity %u", selectedEntity_);
    } else {
        ImGui::TextDisabled("Select an object in Hierarchy");
    }
    ImGui::Separator();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    // FBO dimensions are physical pixels, while ImGui lays out the image in logical pixels.
    const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
    sceneViewportWidth_ = std::max(1, static_cast<int>(std::lround(available.x * framebufferScale.x)));
    sceneViewportHeight_ = std::max(1, static_cast<int>(std::lround(available.y * framebufferScale.y)));

    if (sceneTexture == 0U || available.x <= 1.0F || available.y <= 1.0F) {
        sceneNavigationActive_ = false;
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
        ImGui::TextDisabled("Waiting for the scene renderer...");
        ImGui::End();
        return;
    }

    // Fit the rendered texture inside the panel without stretching the camera view.
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

    ImGuiIO& io = ImGui::GetIO();
    // Navigation and gizmo input only belong to the actual image, not the panel toolbar or empty margins.
    const bool viewportHovered = ImGui::IsItemHovered();
    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const bool rightMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        sceneNavigationActive_ = true;
    }
    const bool windowFocused = glfwGetWindowAttrib(window_, GLFW_FOCUSED) != GLFW_FALSE;
    if (!rightMouseDown || !windowFocused) {
        sceneNavigationActive_ = false;
    }
    if (!windowFocused) {
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
    }
    if (viewportHovered && !sceneNavigationActive_ && !gizmoDragging_ && !io.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) {
            gizmoMode_ = GizmoMode::Translate;
        } else if (ImGui::IsKeyPressed(ImGuiKey_E)) {
            gizmoMode_ = GizmoMode::Rotate;
        } else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            gizmoMode_ = GizmoMode::Scale;
        }
    }

    if (viewportHovered && io.MouseWheel != 0.0F) {
        renderer.MoveCamera(io.MouseWheel, 0.0F, 0.0F, 0.12F);
    }
    if (viewportHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        renderer.MoveCamera(0.0F, -io.MouseDelta.x * 0.02F, io.MouseDelta.y * 0.02F, 0.1F);
    }

    auto* transform = selectedEntity_ != 0U && world.IsAlive(selectedEntity_)
                          ? world.GetComponent<scene::TransformComponent>(selectedEntity_)
                          : nullptr;
    if (transform == nullptr) {
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
    } else {
        const glm::mat4 viewProjection = renderer.ViewProjection();
        const glm::vec3 center = glm::vec3(transform->world[3]);
        const glm::quat orientation = glm::normalize(transform->local.rotation);
        const std::array<glm::vec3, 3> localAxes{glm::vec3{1.0F, 0.0F, 0.0F},
                                                  glm::vec3{0.0F, 1.0F, 0.0F},
                                                  glm::vec3{0.0F, 0.0F, 1.0F}};
        constexpr std::array<ImU32, 3> axisColors{IM_COL32(242, 77, 77, 255), IM_COL32(93, 216, 120, 255),
                                                   IM_COL32(90, 150, 255, 255)};
        constexpr std::array<const char*, 3> axisLabels{"X", "Y", "Z"};
        constexpr float handleLength = 82.0F;
        ImVec2 screenCenter{};
        const bool centerVisible = ProjectToScreen(viewProjection, center, imageMin, imageSize, screenCenter);
        ImVec2 gizmoOrigin = screenCenter;
        auto* drawList = ImGui::GetWindowDrawList();

        const bool centerInsideImage = screenCenter.x >= imageMin.x && screenCenter.x <= imageMin.x + imageSize.x &&
                                       screenCenter.y >= imageMin.y && screenCenter.y <= imageMin.y + imageSize.y;
        if (centerVisible && (centerInsideImage || gizmoDragging_)) {
            std::array<glm::vec2, 3> screenAxes{};
            std::array<glm::vec3, 3> worldAxes{};
            std::array<float, 3> worldPerPixel{};
            std::array<bool, 3> axisVisible{};
            std::array<float, 3> axisHitDistances{std::numeric_limits<float>::max(),
                                                   std::numeric_limits<float>::max(),
                                                   std::numeric_limits<float>::max()};

            for (int axis = 0; axis < 3; ++axis) {
                const auto index = static_cast<std::size_t>(axis);
                worldAxes[index] = glm::normalize(orientation * localAxes[index]);
                ImVec2 unitEndpoint{};
                if (!ProjectToScreen(viewProjection, center + worldAxes[index], imageMin, imageSize, unitEndpoint)) {
                    continue;
                }
                const glm::vec2 projectedUnit{unitEndpoint.x - gizmoOrigin.x, unitEndpoint.y - gizmoOrigin.y};
                const float projectedLength = glm::length(projectedUnit);
                if (gizmoMode_ != GizmoMode::Rotate && projectedLength < 0.001F) {
                    continue;
                }
                if (projectedLength >= 0.001F) {
                    screenAxes[index] = projectedUnit / projectedLength;
                    // One world unit projects to projectedLength pixels; invert it for drag-to-world conversion.
                    worldPerPixel[index] = 1.0F / projectedLength;
                    axisVisible[index] = true;
                }
                if (gizmoMode_ == GizmoMode::Rotate) {
                    axisVisible[index] = true;
                    constexpr int ringSegments = 64;
                    constexpr float ringRadius = 0.9F;
                    const int firstPlaneAxis = (axis + 1) % 3;
                    const int secondPlaneAxis = (axis + 2) % 3;
                    const glm::vec3 firstBasis = orientation * localAxes[static_cast<std::size_t>(firstPlaneAxis)];
                    const glm::vec3 secondBasis = orientation * localAxes[static_cast<std::size_t>(secondPlaneAxis)];
                    ImVec2 previous{};
                    bool previousValid = false;
                    for (int segment = 0; segment <= ringSegments; ++segment) {
                        const float angle = 6.28318530718F * static_cast<float>(segment) /
                                            static_cast<float>(ringSegments);
                        const glm::vec3 ringPoint = center + ringRadius *
                            (firstBasis * std::cos(angle) + secondBasis * std::sin(angle));
                        ImVec2 current{};
                        const bool currentValid = ProjectToScreen(viewProjection, ringPoint, imageMin, imageSize, current);
                        if (currentValid && previousValid) {
                            const float distance = DistanceToSegment(io.MousePos, previous, current);
                            axisHitDistances[index] = std::min(axisHitDistances[index], distance);
                            drawList->AddLine(previous, current, axisColors[index],
                                              gizmoDragging_ && gizmoActiveAxis_ == axis ? 3.5F : 2.0F);
                        }
                        previous = current;
                        previousValid = currentValid;
                    }
                } else {
                    const ImVec2 endpoint(gizmoOrigin.x + screenAxes[index].x * handleLength,
                                          gizmoOrigin.y + screenAxes[index].y * handleLength);
                    float lineFraction = 0.0F;
                    const float lineDistance = DistanceToSegment(io.MousePos, gizmoOrigin, endpoint, &lineFraction);
                    axisHitDistances[index] = lineDistance;
                    if (lineFraction < 0.12F) {
                        axisHitDistances[index] = std::numeric_limits<float>::max();
                    }
                    const bool active = gizmoDragging_ && gizmoActiveAxis_ == axis;
                    drawList->AddLine(gizmoOrigin, endpoint, axisColors[index], active ? 4.0F : 3.0F);
                    if (gizmoMode_ == GizmoMode::Translate) {
                        const glm::vec2 direction = screenAxes[index];
                        const glm::vec2 perpendicular{-direction.y, direction.x};
                        const ImVec2 tip = endpoint;
                        const ImVec2 base(tip.x - direction.x * 11.0F, tip.y - direction.y * 11.0F);
                        drawList->AddTriangleFilled(
                            tip, ImVec2(base.x + perpendicular.x * 5.0F, base.y + perpendicular.y * 5.0F),
                            ImVec2(base.x - perpendicular.x * 5.0F, base.y - perpendicular.y * 5.0F), axisColors[index]);
                    } else {
                        drawList->AddRectFilled(ImVec2(endpoint.x - 5.0F, endpoint.y - 5.0F),
                                                ImVec2(endpoint.x + 5.0F, endpoint.y + 5.0F), axisColors[index]);
                    }
                    drawList->AddText(ImVec2(endpoint.x + 5.0F, endpoint.y + 3.0F), axisColors[index], axisLabels[index]);
                }
            }

            drawList->AddCircleFilled(gizmoOrigin, 5.0F, IM_COL32(240, 240, 245, 255));
            int hoveredAxis = -1;
            float nearestDistance = 10.0F;
            for (int axis = 0; axis < 3; ++axis) {
                const auto index = static_cast<std::size_t>(axis);
                if (axisVisible[index] && axisHitDistances[index] < nearestDistance) {
                    nearestDistance = axisHitDistances[index];
                    hoveredAxis = axis;
                }
            }
            if (viewportHovered && hoveredAxis >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !sceneNavigationActive_) {
                const auto index = static_cast<std::size_t>(hoveredAxis);
                // Anchor the drag at mouse-down so each frame applies the total delta once, without accumulating drift.
                gizmoDragging_ = true;
                gizmoActiveAxis_ = hoveredAxis;
                gizmoDragStartMouse_ = glm::vec2(io.MousePos.x, io.MousePos.y);
                gizmoDragScreenAxis_ = screenAxes[index];
                gizmoDragAxisLocal_ = localAxes[index];
                gizmoDragAxisWorld_ = worldAxes[index];
                gizmoDragStartTranslation_ = transform->local.translation;
                gizmoDragStartRotation_ = transform->local.rotation;
                gizmoDragStartScale_ = transform->local.scale;
                gizmoWorldPerPixel_ = worldPerPixel[index];
                gizmoRotationStartAngle_ = std::atan2(io.MousePos.y - gizmoOrigin.y, io.MousePos.x - gizmoOrigin.x);
            }

            if (gizmoDragging_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                const glm::vec2 mouse(io.MousePos.x, io.MousePos.y);
                if (gizmoMode_ == GizmoMode::Rotate) {
                    const float currentAngle = std::atan2(io.MousePos.y - gizmoOrigin.y,
                                                          io.MousePos.x - gizmoOrigin.x);
                    // Remainder unwraps the 2π boundary, avoiding a full-turn jump when crossing the top of the ring.
                    const float angleDelta = std::remainder(currentAngle - gizmoRotationStartAngle_,
                                                             6.28318530718F);
                    transform->local.rotation = glm::normalize(
                        gizmoDragStartRotation_ * glm::angleAxis(angleDelta, gizmoDragAxisLocal_));
                } else {
                    const glm::vec2 mouseDelta = mouse - gizmoDragStartMouse_;
                    const float axisDelta = glm::dot(mouseDelta, gizmoDragScreenAxis_) * gizmoWorldPerPixel_;
                    if (gizmoMode_ == GizmoMode::Translate) {
                        transform->local.translation = gizmoDragStartTranslation_ + gizmoDragAxisWorld_ * axisDelta;
                    } else {
                        // Keep scales positive so an axis drag cannot invert the object unexpectedly.
                        transform->local.scale[gizmoActiveAxis_] =
                            std::max(0.01F, gizmoDragStartScale_[gizmoActiveAxis_] + axisDelta);
                    }
                }
                transform->dirty = true;
            } else if (gizmoDragging_) {
                gizmoDragging_ = false;
                gizmoActiveAxis_ = -1;
            }

            if (hoveredAxis >= 0 && !gizmoDragging_ && viewportHovered) {
                ImGui::SetMouseCursor(gizmoMode_ == GizmoMode::Rotate ? ImGuiMouseCursor_Hand
                                                                       : ImGuiMouseCursor_ResizeAll);
            }
        }
    }
    if (gizmoDragging_ && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        gizmoDragging_ = false;
        gizmoActiveAxis_ = -1;
    }

    if (sceneNavigationActive_) {
        renderer.RotateCamera(io.MouseDelta.x, io.MouseDelta.y);

        const float forward = (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS ? 1.0F : 0.0F) -
                              (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS ? 1.0F : 0.0F);
        const float right = (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS ? 1.0F : 0.0F) -
                            (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS ? 1.0F : 0.0F);
        const float up = (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS ? 1.0F : 0.0F) -
                         (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS ? 1.0F : 0.0F);
        const bool fast = glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                          glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const float speedMultiplier = fast ? 4.0F : 1.0F;
        const float safeDeltaSeconds = std::clamp(deltaSeconds, 0.0F, 0.1F);
        renderer.MoveCamera(forward, right, up, safeDeltaSeconds * speedMultiplier);
    }
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
    ImGui::InputTextWithHint("##AssetFilter", "Filter assets", assetFilter_.data(), assetFilter_.size());
    ImGui::SetNextItemWidth(180.0F);
    ImGui::InputTextWithHint("##NewScriptName", "New Lua script", newScriptName_.data(), newScriptName_.size());
    ImGui::SameLine();
    if (ImGui::Button("Create Script")) {
        CreateLuaScript();
    }
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
            if (selectedPath.extension() == ".lua") {
                if (scriptEditorAsset_ != selectedAsset_) {
                    std::ifstream scriptFile(selectedPath);
                    std::string source((std::istreambuf_iterator<char>(scriptFile)), std::istreambuf_iterator<char>());
                    scriptEditorBuffer_.fill('\0');
                    // The editor uses a fixed buffer to avoid reallocating user input every keystroke.
                    scriptEditorTooLarge_ = source.size() >= scriptEditorBuffer_.size();
                    const std::size_t copied = std::min(source.size(), scriptEditorBuffer_.size() - 1U);
                    std::copy_n(source.data(), copied, scriptEditorBuffer_.data());
                    scriptEditorAsset_ = selectedAsset_;
                    scriptEditorDirty_ = false;
                }
                ImGui::TextDisabled("Lua source");
                if (scriptEditorTooLarge_) {
                    ImGui::TextColored(ImVec4(1.0F, 0.55F, 0.35F, 1.0F),
                                       "This file exceeds the 32 KB editor limit and is read-only here.");
                }
                ImGui::BeginDisabled(scriptEditorTooLarge_);
                if (ImGui::InputTextMultiline("##LuaSource", scriptEditorBuffer_.data(), scriptEditorBuffer_.size(),
                                              ImVec2(-1.0F, 210.0F), ImGuiInputTextFlags_AllowTabInput |
                                                                          (scriptEditorTooLarge_ ? ImGuiInputTextFlags_ReadOnly : 0))) {
                    scriptEditorDirty_ = true;
                }
                if (ImGui::Button(scriptEditorDirty_ ? "Save Lua Script*" : "Save Lua Script")) {
                    std::ofstream scriptFile(selectedPath, std::ios::trunc);
                    if (scriptFile.is_open()) {
                        scriptFile << scriptEditorBuffer_.data();
                        scriptEditorDirty_ = !scriptFile.good();
                        Log(scriptEditorDirty_ ? "Could not save " + selectedAsset_ : "Saved " + selectedAsset_ + ".");
                    } else {
                        Log("Could not open " + selectedAsset_ + " for writing.");
                    }
                }
                ImGui::EndDisabled();
            } else {
                ImGui::TextDisabled("Preview is not available for this file type yet.");
                scriptEditorAsset_.clear();
                scriptEditorDirty_ = false;
                scriptEditorTooLarge_ = false;
            }
        }
    }
    ImGui::End();
}

void EditorLayer::DrawAssetDirectory(const std::filesystem::path& directory) {
    std::error_code error;
    std::vector<std::filesystem::directory_entry> entries{};
    for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied,
                                                error),
         end;
         !error && it != end; it.increment(error)) {
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

    std::string filter = assetFilter_.data();
    std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& entry : entries) {
        const std::string relative = entry.path().lexically_relative(assetRoot_).generic_string();
        std::error_code entryError;
        if (entry.is_symlink(entryError)) {
            // Do not let a project link send the browser outside the selected content root.
            continue;
        }
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

std::vector<std::string> EditorLayer::AvailableLuaScripts() const {
    std::vector<std::string> scripts{};
    const std::filesystem::path scriptsDirectory = assetRoot_ / "scripts";
    std::error_code error;
    if (!std::filesystem::is_directory(scriptsDirectory, error)) {
        return scripts;
    }

    std::filesystem::recursive_directory_iterator iterator(
        scriptsDirectory, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end{};
    while (!error && iterator != end) {
        if (iterator->is_symlink(error)) {
            iterator.disable_recursion_pending();
        } else if (iterator->is_regular_file(error) && iterator->path().extension() == ".lua") {
            scripts.push_back(iterator->path().lexically_relative(assetRoot_).generic_string());
        }
        iterator.increment(error);
    }
    std::sort(scripts.begin(), scripts.end());
    return scripts;
}

void EditorLayer::CreateLuaScript() {
    std::string name = newScriptName_.data();
    if (name.ends_with(".lua")) {
        name.resize(name.size() - 4U);
    }
    // A simple filename keeps new scripts inside the scripts folder without path parsing surprises.
    const bool validName = !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '_' || character == '-';
    });
    if (!validName) {
        Log("Use only letters, numbers, underscores, or hyphens for a Lua script name.");
        return;
    }

    const std::filesystem::path scriptsDirectory = assetRoot_ / "scripts";
    std::error_code error;
    std::filesystem::create_directories(scriptsDirectory, error);
    if (error) {
        Log("Could not create the scripts folder: " + error.message());
        return;
    }
    const std::filesystem::path scriptPath = scriptsDirectory / (name + ".lua");
    if (std::filesystem::exists(scriptPath, error)) {
        Log("A script named " + name + " already exists.");
        return;
    }

    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        Log("Could not create " + scriptPath.filename().string() + ".");
        return;
    }
    scriptFile << "-- Update runs once per fixed simulation step for the attached object.\n"
                  "-- Edit the transform table to change this object's local transform.\n\n"
                  "function Update(entity, dt, transform)\n"
                  "    -- Example: transform.y = transform.y + dt\n"
                  "end\n";
    if (!scriptFile.good()) {
        Log("Could not write the new Lua script.");
        return;
    }

    std::fill(newScriptName_.begin(), newScriptName_.end(), '\0');
    selectedAsset_ = scriptPath.lexically_relative(assetRoot_).generic_string();
    scriptEditorAsset_.clear();
    Log("Created " + selectedAsset_ + ". Select an object and attach it from Inspector.");
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
    const std::string objectName = name + " " + std::to_string(objectCount + 1U);
    world.AddComponent(entity, scene::NameComponent{objectName});
    Log("Created " + objectName + ".");
}

void EditorLayer::DeleteSelectedEntity(ecs::ECSWorld& world) {
    if (selectedEntity_ == 0U || !world.IsAlive(selectedEntity_)) {
        return;
    }

    ecs::Entity parent = 0U;
    if (const auto* node = world.GetComponent<scene::SceneNodeComponent>(selectedEntity_)) {
        parent = node->parent;
    }
    // Reparent direct children before deleting this node so none keep a dangling parent ID.
    world.ForEach<scene::SceneNodeComponent, scene::TransformComponent>(
        [&](ecs::Entity entity, scene::SceneNodeComponent& node, scene::TransformComponent& transform) {
            if (entity != selectedEntity_ && node.parent == selectedEntity_) {
                node.parent = parent;
                transform.dirty = true;
            }
        });

    const ecs::Entity removed = selectedEntity_;
    world.DestroyEntity(removed);
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
    // The editor console is a live log, not an archive; cap it so long sessions do not grow without bound.
    constexpr std::size_t maxEntries = 200;
    if (consoleMessages_.size() > maxEntries) {
        consoleMessages_.erase(consoleMessages_.begin(), consoleMessages_.begin() +
                                                          static_cast<std::ptrdiff_t>(consoleMessages_.size() - maxEntries));
    }
}

} // namespace axiom::editor
