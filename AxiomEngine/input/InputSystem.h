#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace axiom::input {

struct InputAction {
    bool pressed{false};
    bool justPressed{false};
    bool justReleased{false};
    float value{0.0F};
};

enum class InputBindingType {
    Key,
    Axis,
};

struct InputBinding {
    InputBindingType type{InputBindingType::Key};
    int positiveKey{0};
    int negativeKey{0};
    float scale{1.0F};
    float deadzone{0.0F};
    float curveExponent{1.0F};
};

class InputSystem {
  public:
    void Update();

    void SetActionState(const std::string& actionName, bool pressed, float value = 1.0F);
    void AddKeyBinding(const std::string& actionName, int keyCode, float scale = 1.0F);
    void AddAxisBinding(const std::string& actionName, int negativeKeyCode, int positiveKeyCode, float scale = 1.0F);
    void SetActionDeadzone(const std::string& actionName, float deadzone);
    void SetActionCurveExponent(const std::string& actionName, float curveExponent);
    void ClearBindings();
    void EvaluateBindings(const std::unordered_map<int, bool>& keyState);
    bool SaveActionMap(const std::string& path) const;
    bool LoadActionMap(const std::string& path);

    [[nodiscard]] bool IsPressed(const std::string& actionName) const;
    [[nodiscard]] bool WasPressed(const std::string& actionName) const;
    [[nodiscard]] bool WasReleased(const std::string& actionName) const;
    [[nodiscard]] float Value(const std::string& actionName) const;

  private:
    struct ActionMapEntry {
        float deadzone{0.0F};
        float curveExponent{1.0F};
        std::vector<InputBinding> bindings{};
    };

    [[nodiscard]] static float ApplyDeadzoneAndCurve(float value, float deadzone, float curveExponent);

    std::unordered_map<std::string, InputAction> actions_{};
    std::unordered_map<std::string, ActionMapEntry> actionMap_{};
};

} // namespace axiom::input
