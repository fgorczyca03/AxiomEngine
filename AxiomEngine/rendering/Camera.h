#pragma once

#include <glm/glm.hpp>

namespace axiom::rendering {

class Camera {
  public:
    void SetPerspective(float fovRadians, float aspect, float nearPlane, float farPlane);
    void Move(float forward, float right, float up, float deltaSeconds);
    void Rotate(float deltaX, float deltaY);

    [[nodiscard]] glm::mat4 ViewProjection() const;
    [[nodiscard]] glm::vec3 Position() const;

  private:
    [[nodiscard]] glm::vec3 Forward() const;
    [[nodiscard]] glm::vec3 Right() const;

    glm::vec3 position_{0.0F, 1.5F, 3.5F};
    float yawDegrees_{-90.0F};
    float pitchDegrees_{-23.19859F};
    float movementSpeed_{5.0F};
    float lookSensitivity_{0.12F};
    glm::mat4 projection_{1.0F};
};

} // namespace axiom::rendering
