#pragma once

#include <bitset>
#include <cstddef>
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

// A named snapshot lets replay files stay independent of unordered_map iteration order.
struct NamedInputAction {
    std::string name{};
    InputAction state{};
};

enum class InputBindingType {
    Key,
    Axis,
    MouseAxis,
    GamepadAxis,
    GamepadButton,
};

struct InputBinding {
    InputBindingType type{InputBindingType::Key};
    int positiveKey{0};
    int negativeKey{0};
    float scale{1.0F};
    float deadzone{0.0F};
    float curveExponent{1.0F};
};

struct InputBindingDescription {
    std::string actionName{};
    InputBindingType type{InputBindingType::Key};
    int positiveCode{0};
    int negativeCode{0};
    float scale{1.0F};
};

struct InputSnapshot {
    static constexpr std::size_t KeyCodeCount = 512;

    std::bitset<KeyCodeCount> keyState{};
    std::unordered_map<int, float> mouseAxisState{};
    std::unordered_map<int, bool> gamepadButtonState{};
    std::unordered_map<int, float> gamepadAxisState{};

    void SetKeyState(int keyCode, bool pressed) {
        if (keyCode >= 0 && static_cast<std::size_t>(keyCode) < KeyCodeCount) {
            keyState.set(static_cast<std::size_t>(keyCode), pressed);
        }
    }

    [[nodiscard]] bool IsKeyPressed(int keyCode) const {
        return keyCode >= 0 && static_cast<std::size_t>(keyCode) < KeyCodeCount &&
               keyState.test(static_cast<std::size_t>(keyCode));
    }

    void ClearKeyState() { keyState.reset(); }
};

class InputSystem {
  public:
    void Update();

    void SetActionState(const std::string& actionName, bool pressed, float value = 1.0F);
    void AddKeyBinding(const std::string& actionName, int keyCode, float scale = 1.0F);
    void AddAxisBinding(const std::string& actionName, int negativeKeyCode, int positiveKeyCode, float scale = 1.0F);
    void AddMouseAxisBinding(const std::string& actionName, int axisId, float scale = 1.0F);
    void AddGamepadAxisBinding(const std::string& actionName, int axisId, float scale = 1.0F);
    void AddGamepadButtonBinding(const std::string& actionName, int buttonId, float scale = 1.0F);
    void SetActionDeadzone(const std::string& actionName, float deadzone);
    void SetActionCurveExponent(const std::string& actionName, float curveExponent);
    void ClearBindings();
    void EvaluateBindings(const std::unordered_map<int, bool>& keyState);
    void EvaluateBindings(const InputSnapshot& snapshot);
    bool SaveActionMap(const std::string& path) const;
    bool LoadActionMap(const std::string& path);

    [[nodiscard]] bool IsPressed(const std::string& actionName) const;
    [[nodiscard]] bool WasPressed(const std::string& actionName) const;
    [[nodiscard]] bool WasReleased(const std::string& actionName) const;
    [[nodiscard]] float Value(const std::string& actionName) const;
    [[nodiscard]] std::vector<std::string> ActionNames() const;
    [[nodiscard]] std::vector<InputBindingDescription> DescribeBindings(const std::string& actionName) const;
    // CopyActionStates writes a name-sorted snapshot; ApplyActionStates restores one for playback.
    void CopyActionStates(std::vector<NamedInputAction>& destination) const;
    void ApplyActionStates(const std::vector<NamedInputAction>& states);

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
