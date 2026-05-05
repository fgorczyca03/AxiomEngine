#include "AxiomEngine/input/InputSystem.h"

namespace axiom::input {

void InputSystem::Update() {
    for (auto& [_, action] : actions_) {
        action.justPressed = false;
        action.justReleased = false;
    }
}

void InputSystem::SetActionState(const std::string& actionName, bool pressed, float value) {
    InputAction& action = actions_[actionName];
    const bool wasPressed = action.pressed;
    action.pressed = pressed;
    action.justPressed = !wasPressed && pressed;
    action.justReleased = wasPressed && !pressed;
    action.value = pressed ? value : 0.0F;
}

bool InputSystem::IsPressed(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    if (it == actions_.end()) {
        return false;
    }
    return it->second.pressed;
}

bool InputSystem::WasPressed(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    if (it == actions_.end()) {
        return false;
    }
    return it->second.justPressed;
}

bool InputSystem::WasReleased(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    if (it == actions_.end()) {
        return false;
    }
    return it->second.justReleased;
}

float InputSystem::Value(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    if (it == actions_.end()) {
        return 0.0F;
    }
    return it->second.value;
}

} // namespace axiom::input
