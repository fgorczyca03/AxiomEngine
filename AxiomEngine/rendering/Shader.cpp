#include "AxiomEngine/rendering/Shader.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <string>

namespace axiom::rendering {

namespace {
std::string ReadText(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

bool CheckShader(unsigned int shader) {
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return success == GL_TRUE;
}

bool CheckProgram(unsigned int program) {
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    return success == GL_TRUE;
}

unsigned int Compile(GLenum type, const std::string& src) {
    if (src.empty()) {
        return 0U;
    }

    const auto shader = glCreateShader(type);
    const char* ptr = src.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);

    if (!CheckShader(shader)) {
        glDeleteShader(shader);
        return 0U;
    }

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
    if (vertex == 0U || fragment == 0U) {
        if (vertex != 0U) {
            glDeleteShader(vertex);
        }
        if (fragment != 0U) {
            glDeleteShader(fragment);
        }
        return false;
    }

    const auto program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!CheckProgram(program)) {
        glDeleteProgram(program);
        return false;
    }

    if (program_ != 0U) {
        glDeleteProgram(program_);
    }
    program_ = program;
    return true;
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

void Shader::SetFloat(const std::string& name, float value) const {
    const auto loc = glGetUniformLocation(program_, name.c_str());
    glUniform1f(loc, value);
}

} // namespace axiom::rendering
