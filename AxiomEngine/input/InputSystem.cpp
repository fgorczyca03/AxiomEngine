#include "AxiomEngine/input/InputSystem.h"

namespace axiom::input {

void InputSystem::Update() {
    // Placeholder for platform polling. Keeps held action values stable.
}

void InputSystem::SetActionState(const std::string& actionName, bool pressed, float value) {
    InputAction& action = actions_[actionName];
    action.pressed = pressed;
    action.value = pressed ? value : 0.0F;
}

bool InputSystem::IsPressed(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    if (it == actions_.end()) {
        return false;
    }
    return it->second.pressed;
}

float InputSystem::Value(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    if (it == actions_.end()) {
        return 0.0F;
    }
    return it->second.value;
}

} // namespace axiom::input
