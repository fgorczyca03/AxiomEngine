#pragma once

namespace axiom::editor {

struct EditorFrameInfo {
    float deltaSeconds{0.0F};
};

class EditorLayer {
  public:
    bool Initialize();
    void Shutdown();

    void BeginFrame();
    void Draw(const EditorFrameInfo& frameInfo);
    void EndFrame();

  private:
    bool initialized_{false};
};

} // namespace axiom::editor
