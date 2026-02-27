#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace axiom::math {

struct Transform {
    glm::vec3 translation{0.0F};
    glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
    glm::vec3 scale{1.0F};

    [[nodiscard]] glm::mat4 LocalMatrix() const {
        return glm::translate(glm::mat4(1.0F), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0F), scale);
    }
};

} // namespace axiom::math
