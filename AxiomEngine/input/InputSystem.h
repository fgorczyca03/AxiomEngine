#pragma once

#include <string>
#include <unordered_map>

namespace axiom::input {

struct InputAction {
    bool pressed{false};
    float value{0.0F};
};

class InputSystem {
  public:
    void Update();

    void SetActionState(const std::string& actionName, bool pressed, float value = 1.0F);
    [[nodiscard]] bool IsPressed(const std::string& actionName) const;
    [[nodiscard]] float Value(const std::string& actionName) const;

  private:
    std::unordered_map<std::string, InputAction> actions_{};
};

} // namespace axiom::input
