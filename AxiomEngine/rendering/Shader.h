#pragma once

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

namespace axiom::rendering {

class Shader {
  public:
    Shader() = default;
    ~Shader();

    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void Bind() const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;

  private:
    [[nodiscard]] int UniformLocation(const std::string& name) const;

    unsigned int program_{0};
    mutable std::unordered_map<std::string, int> uniformLocations_{};
};

} // namespace axiom::rendering
