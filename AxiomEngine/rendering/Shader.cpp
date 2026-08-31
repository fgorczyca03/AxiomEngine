#include "AxiomEngine/rendering/Shader.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

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

void PrintShaderLog(unsigned int shader, const std::string& path) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return;
    }

    std::vector<char> log(static_cast<std::size_t>(length));
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    std::cerr << "Axiom shader compile error in " << path << ": " << log.data() << '\n';
}

void PrintProgramLog(unsigned int program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return;
    }

    std::vector<char> log(static_cast<std::size_t>(length));
    glGetProgramInfoLog(program, length, nullptr, log.data());
    std::cerr << "Axiom shader link error: " << log.data() << '\n';
}

unsigned int Compile(GLenum type, const std::string& source, const std::string& path) {
    if (source.empty()) {
        std::cerr << "Axiom shader source is empty or missing: " << path << '\n';
        return 0U;
    }

    const auto shader = glCreateShader(type);
    const char* ptr = source.c_str();
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        PrintShaderLog(shader, path);
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
    const auto vertex = Compile(GL_VERTEX_SHADER, ReadText(vertexPath), vertexPath);
    const auto fragment = Compile(GL_FRAGMENT_SHADER, ReadText(fragmentPath), fragmentPath);
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
    if (program == 0U) {
        std::cerr << "Axiom shader program creation failed" << '\n';
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        PrintProgramLog(program);
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
