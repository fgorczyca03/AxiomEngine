#include "AxiomEngine/rendering/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace axiom::rendering {

void Camera::SetPerspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
    projection_ = glm::perspective(fovRadians, aspect, nearPlane, farPlane);
}

glm::mat4 Camera::ViewProjection() const {
    const auto view = glm::lookAt(position_, target_, glm::vec3(0.0F, 1.0F, 0.0F));
    return projection_ * view;
}

} // namespace axiom::rendering
