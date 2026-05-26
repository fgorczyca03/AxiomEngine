#include "AxiomEngine/input/InputSystem.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace axiom::input {

namespace {
float Clamp01(float value) { return std::clamp(value, 0.0F, 1.0F); }
} // namespace

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

void InputSystem::AddKeyBinding(const std::string& actionName, int keyCode, float scale) {
    actionMap_[actionName].bindings.push_back({InputBindingType::Key, keyCode, 0, scale, 0.0F, 1.0F});
}

void InputSystem::AddAxisBinding(const std::string& actionName, int negativeKeyCode, int positiveKeyCode, float scale) {
    actionMap_[actionName].bindings.push_back({InputBindingType::Axis, positiveKeyCode, negativeKeyCode, scale, 0.0F, 1.0F});
}

void InputSystem::AddMouseAxisBinding(const std::string& actionName, int axisId, float scale) {
    actionMap_[actionName].bindings.push_back({InputBindingType::MouseAxis, axisId, 0, scale, 0.0F, 1.0F});
}

void InputSystem::AddGamepadAxisBinding(const std::string& actionName, int axisId, float scale) {
    actionMap_[actionName].bindings.push_back({InputBindingType::GamepadAxis, axisId, 0, scale, 0.0F, 1.0F});
}

void InputSystem::AddGamepadButtonBinding(const std::string& actionName, int buttonId, float scale) {
    actionMap_[actionName].bindings.push_back({InputBindingType::GamepadButton, buttonId, 0, scale, 0.0F, 1.0F});
}

void InputSystem::SetActionDeadzone(const std::string& actionName, float deadzone) { actionMap_[actionName].deadzone = Clamp01(deadzone); }

void InputSystem::SetActionCurveExponent(const std::string& actionName, float curveExponent) {
    actionMap_[actionName].curveExponent = std::max(curveExponent, 0.01F);
}

void InputSystem::ClearBindings() {
    actionMap_.clear();
    actions_.clear();
}

void InputSystem::EvaluateBindings(const std::unordered_map<int, bool>& keyState) {
    InputSnapshot snapshot{};
    snapshot.keyState = keyState;
    EvaluateBindings(snapshot);
}

void InputSystem::EvaluateBindings(const InputSnapshot& snapshot) {
    for (const auto& [actionName, entry] : actionMap_) {
        float combinedValue = 0.0F;
        for (const InputBinding& binding : entry.bindings) {
            float sample = 0.0F;
            if (binding.type == InputBindingType::Key) {
                const auto it = snapshot.keyState.find(binding.positiveKey);
                sample = (it != snapshot.keyState.end() && it->second) ? binding.scale : 0.0F;
            } else if (binding.type == InputBindingType::Axis) {
                const bool posPressed = snapshot.keyState.contains(binding.positiveKey) && snapshot.keyState.at(binding.positiveKey);
                const bool negPressed = snapshot.keyState.contains(binding.negativeKey) && snapshot.keyState.at(binding.negativeKey);
                sample = (posPressed ? 1.0F : 0.0F) - (negPressed ? 1.0F : 0.0F);
                sample *= binding.scale;
            } else if (binding.type == InputBindingType::MouseAxis) {
                if (snapshot.mouseAxisState.contains(binding.positiveKey)) {
                    sample = snapshot.mouseAxisState.at(binding.positiveKey) * binding.scale;
                }
            } else if (binding.type == InputBindingType::GamepadAxis) {
                if (snapshot.gamepadAxisState.contains(binding.positiveKey)) {
                    sample = snapshot.gamepadAxisState.at(binding.positiveKey) * binding.scale;
                }
            } else if (binding.type == InputBindingType::GamepadButton) {
                const bool pressed = snapshot.gamepadButtonState.contains(binding.positiveKey) && snapshot.gamepadButtonState.at(binding.positiveKey);
                sample = pressed ? binding.scale : 0.0F;
            }
            combinedValue += sample;
        }

        combinedValue = std::clamp(combinedValue, -1.0F, 1.0F);
        const float processed = ApplyDeadzoneAndCurve(combinedValue, entry.deadzone, entry.curveExponent);
        SetActionState(actionName, std::abs(processed) > 0.0001F, processed);
    }
}

bool InputSystem::SaveActionMap(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "AXIOM_ACTION_MAP_V1\n";
    for (const auto& [actionName, entry] : actionMap_) {
        out << "ACTION " << actionName << " " << entry.deadzone << " " << entry.curveExponent << "\n";
        for (const InputBinding& binding : entry.bindings) {
            std::string type = "KEY";
            if (binding.type == InputBindingType::Axis) type = "AXIS";
            if (binding.type == InputBindingType::MouseAxis) type = "MOUSE_AXIS";
            if (binding.type == InputBindingType::GamepadAxis) type = "GAMEPAD_AXIS";
            if (binding.type == InputBindingType::GamepadButton) type = "GAMEPAD_BUTTON";
            out << "BIND " << type << " " << binding.positiveKey << " " << binding.negativeKey << " " << binding.scale << "\n";
        }
        out << "END_ACTION\n";
    }
    return true;
}

bool InputSystem::LoadActionMap(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string header;
    if (!std::getline(in, header) || header != "AXIOM_ACTION_MAP_V1") {
        return false;
    }

    std::unordered_map<std::string, ActionMapEntry> loaded;
    std::string line;
    std::string currentAction;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;
        if (tag == "ACTION") {
            ActionMapEntry entry{};
            stream >> currentAction >> entry.deadzone >> entry.curveExponent;
            loaded[currentAction] = entry;
        } else if (tag == "BIND" && !currentAction.empty()) {
            std::string type;
            InputBinding binding{};
            stream >> type >> binding.positiveKey >> binding.negativeKey >> binding.scale;
            binding.type = InputBindingType::Key;
            if (type == "AXIS") binding.type = InputBindingType::Axis;
            else if (type == "MOUSE_AXIS") binding.type = InputBindingType::MouseAxis;
            else if (type == "GAMEPAD_AXIS") binding.type = InputBindingType::GamepadAxis;
            else if (type == "GAMEPAD_BUTTON") binding.type = InputBindingType::GamepadButton;
            loaded[currentAction].bindings.push_back(binding);
        } else if (tag == "END_ACTION") {
            currentAction.clear();
        }
    }

    actionMap_ = std::move(loaded);
    return true;
}

bool InputSystem::IsPressed(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    return it != actions_.end() && it->second.pressed;
}
bool InputSystem::WasPressed(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    return it != actions_.end() && it->second.justPressed;
}
bool InputSystem::WasReleased(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    return it != actions_.end() && it->second.justReleased;
}
float InputSystem::Value(const std::string& actionName) const {
    const auto it = actions_.find(actionName);
    return it == actions_.end() ? 0.0F : it->second.value;
}

float InputSystem::ApplyDeadzoneAndCurve(float value, float deadzone, float curveExponent) {
    const float magnitude = std::abs(value);
    if (magnitude <= deadzone) {
        return 0.0F;
    }
    const float sign = value < 0.0F ? -1.0F : 1.0F;
    const float normalized = (magnitude - deadzone) / (1.0F - deadzone);
    return sign * std::pow(std::clamp(normalized, 0.0F, 1.0F), std::max(curveExponent, 0.01F));
}

} // namespace axiom::input
