#include "AxiomEngine/rendering/Shader.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

namespace axiom::rendering {

namespace {
std::string ReadText(const std::string& path) {
    std::ifstream file(path);
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

unsigned int Compile(GLenum type, const std::string& src) {
    const auto shader = glCreateShader(type);
    const char* ptr = src.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);
    return shader;
}
} // namespace

Shader::~Shader() {
    if (program_ != 0U) {
        glDeleteProgram(program_);
    }
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    const auto vertex = Compile(GL_VERTEX_SHADER, ReadText(vertexPath));
    const auto fragment = Compile(GL_FRAGMENT_SHADER, ReadText(fragmentPath));
    program_ = glCreateProgram();
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program_ != 0U;
}

void Shader::Bind() const { glUseProgram(program_); }

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    const auto loc = glGetUniformLocation(program_, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    const auto loc = glGetUniformLocation(program_, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

} // namespace axiom::rendering
