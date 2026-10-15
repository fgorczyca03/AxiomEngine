#include "AxiomEngine/rendering/Camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace axiom::rendering {

void Camera::SetPerspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
    projection_ = glm::perspective(fovRadians, aspect, nearPlane, farPlane);
}

void Camera::Move(float forward, float right, float up, float deltaSeconds) {
    if (deltaSeconds <= 0.0F) {
        return;
    }

    glm::vec3 movement = Forward() * forward + Right() * right + glm::vec3(0.0F, up, 0.0F);
    const float movementLength = glm::length(movement);
    if (movementLength > 1.0F) {
        movement /= movementLength;
    }
    position_ += movement * movementSpeed_ * deltaSeconds;
}

void Camera::Rotate(float deltaX, float deltaY) {
    yawDegrees_ += deltaX * lookSensitivity_;
    pitchDegrees_ = std::clamp(pitchDegrees_ - deltaY * lookSensitivity_, -89.0F, 89.0F);
}

glm::mat4 Camera::ViewProjection() const {
    const auto view = glm::lookAt(position_, position_ + Forward(), glm::vec3(0.0F, 1.0F, 0.0F));
    return projection_ * view;
}

glm::vec3 Camera::Position() const { return position_; }

glm::vec3 Camera::Forward() const {
    const float yaw = glm::radians(yawDegrees_);
    const float pitch = glm::radians(pitchDegrees_);
    return glm::normalize(glm::vec3{
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch),
    });
}

glm::vec3 Camera::Right() const {
    return glm::normalize(glm::cross(Forward(), glm::vec3(0.0F, 1.0F, 0.0F)));
}

} // namespace axiom::rendering
