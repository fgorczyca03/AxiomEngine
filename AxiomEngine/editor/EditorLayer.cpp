#include "AxiomEngine/editor/EditorLayer.h"

#include "AxiomEngine/profiling/Profiling.h"

#ifdef AXIOM_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace axiom::editor {

bool EditorLayer::Initialize() {
    AXIOM_PROFILE_FUNCTION();
    initialized_ = true;
    return initialized_;
}

void EditorLayer::Shutdown() {
    AXIOM_PROFILE_FUNCTION();
    initialized_ = false;
}

void EditorLayer::BeginFrame() {
    if (!initialized_) {
        return;
    }
#ifdef AXIOM_ENABLE_IMGUI
    ImGui::NewFrame();
#endif
}

void EditorLayer::Draw(const EditorFrameInfo& frameInfo) {
    if (!initialized_) {
        return;
    }
    (void)frameInfo;
#ifdef AXIOM_ENABLE_IMGUI
    ImGui::Begin("Axiom Editor");
    ImGui::Text("Editor scaffold active");
    ImGui::Text("Frame dt: %.3f ms", frameInfo.deltaSeconds * 1000.0F);
    ImGui::End();
#endif
}

void EditorLayer::EndFrame() {
    if (!initialized_) {
        return;
    }
#ifdef AXIOM_ENABLE_IMGUI
    ImGui::Render();
#endif
}

} // namespace axiom::editor
