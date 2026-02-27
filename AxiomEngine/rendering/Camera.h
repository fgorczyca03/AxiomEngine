#pragma once

#include <glm/glm.hpp>

namespace axiom::rendering {

class Camera {
  public:
    void SetPerspective(float fovRadians, float aspect, float nearPlane, float farPlane);
    [[nodiscard]] glm::mat4 ViewProjection() const;

  private:
    glm::vec3 position_{0.0F, 1.5F, 3.5F};
    glm::vec3 target_{0.0F, 0.0F, 0.0F};
    glm::mat4 projection_{1.0F};
};

} // namespace axiom::rendering
